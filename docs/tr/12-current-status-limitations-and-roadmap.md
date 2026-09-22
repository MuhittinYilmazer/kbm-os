# 12 — Mevcut durum, sınırlar ve yol haritası

KBM şu anda “çalışan genel amaçlı işletim sistemi” değildir. Bu olumsuz bir hüküm değil; doğru mühendislik tanımıdır. KBM, x86-64 kernel bring-up'ın temel halkalarını gerçekten yürütmüş, öğrenme amaçlı bir kernel başlangıcıdır.

## 1. Şu anda gerçekten çalışanlar

Aşağıdaki maddeler kaynakta var ve QEMU'da doğrulanabilir:

| Katman | KBM'nin durumu |
| --- | --- |
| Önyükleme | Limine kernel ELF'ini yükler ve `kmain`e geçer. |
| Seri çıktı | COM1 üzerinden boot ve hata işaretçileri yazar. |
| Ekran ve console | Framebuffer üzerinde 8×8 fontla metin, satır sarma, scroll, clear ve backspace çalışır. |
| Temel C rutinleri | `memcpy`, `memset`, `memmove`, `memcmp` kernel içinde sağlanır. |
| CPU başlangıcı | Minimal GDT yüklenir; code/data selector'ları ayarlanır. |
| Exception altyapısı | IDT'de seçili exception vector'leri ve fatal handler'lar vardır. |
| Donanım interrupt altyapısı | IDT, assembly ISR stub'ları, 8259 PIC remap/mask/EOI işlemleri vardır. |
| Zamanlayıcı | PIT yaklaşık 100 Hz'de IRQ0 üretir; timer sayacı artar. |
| LAPIC | LAPIC MMIO sayfası özel sanal adrese eşlenir; LINT0 üzerinden legacy PIC interrupt yolu açılır. |
| Sayfalama inceleme aracı | CR3'ten başlayan 4 KiB page-table yürüyüşü ve sanal→fiziksel çeviri bulunur. |
| Bellek keşfi | Limine fiziksel memory map'i seri porta yazdırılır. |
| PMM | İki bitmapli 4 KiB frame allocation/free ve ownership kontrolü çalışır. |
| Heap | 16-byte hizalı, PMM destekli küçük free-list allocator; `kmalloc`, `kfree` ve bitişik boş blok birleştirme çalışır. |
| Klavye ve shell | PS/2 IRQ1, Türkçe-Q alt kümesi, scroll eden framebuffer shell, uptime çıktısı ve reboot komutu çalışır. |

“Var” ile “tam üretim kalitesinde” aynı şey değildir. Örneğin exception desteği birkaç vector ile sınırlı, timer sadece tick sayıyor ve framebuffer kodu henüz bir terminal/GUI değildir.

## 2. Henüz olmayanlar

Şu başlıkların olmaması beklenen ve bilinçli bir sonraki iş listesidir:

- Genel amaçlı sayfa ayırma ve map/unmap API'si
- Dosya sistemi, disk sürücüsü, VFS
- Kullanıcı modu, süreçler, scheduler veya syscall ABI'si
- ELF kullanıcı programı yükleme
- Ağ yığını
- Çok çekirdek (SMP), AP başlatma
- Güvenlik sınırları, kullanıcı adres alanı ve erişim denetimi
- Kalıcı test altyapısı ve debugger entegrasyonu

Bu liste “hepsini yapmak zorundasın” listesi değildir. Hobi kernelinde kapsam seçmek başarının parçasıdır.

## 3. Yol haritası: küçük, doğrulanabilir kilometre taşları

### Milestone A — Fiziksel bellek yönetimi (tamamlandı: v0.1 kapsamı)

Amaç: güvenli kullanılabilir RAM'den 4 KiB frame alabilmek.

1. Limine memory map'teki `USABLE` aralıkları incele.
2. Geçici fiziksel bump allocator yaz.
3. Ayrılan birkaç frame'i serial logda göster.
4. Bitmap allocator ile ayırma ve serbest bırakma ekle.
5. Kernel, bootloader, framebuffer ve bitmap'in kendi alanını rezervle.

Bu aşama bittiğinde “kernel hangi fiziksel RAM'i kullanabilir?” sorusuna kodla cevap vermiş olursun.

### Milestone B — Kernel heap ve dinamik veri yapıları (temel free-list sürümü tamamlandı)

Amaç: kernelin yalnızca statik dizilerle yaşamak zorunda kalmaması.

1. Fiziksel allocator'dan frame al.
2. Frame'leri seçilmiş kernel sanal alanına map et.
3. İlk basit heap'i kur.
4. Hizalama, yeni frame alma ve kullanım sayacını küçük testlerle doğrula.
5. Free-list, `kfree` ve bitişik boş blok birleştirme ekle.

Heap allocatorı başta basit tutmak akıllıcadır. Amaç erken aşamada maksimum performans değil, sahiplik ve hata davranışını anlamaktır.

### Milestone C — İnsanla etkileşim (temel sürüm tamamlandı)

Amaç: kullanıcıya seri terminal olmadan anlamlı geri bildirim vermek.

1. Framebuffer üzerinde metin çizimi, satır sarma ve scroll.
2. Türkçe-Q alt kümeli PS/2 klavye girişi.
3. Yazma, backspace ve komut satırı olan küçük bir kernel console.
4. `HELP`, `MEM`, `TICKS`, `UPTIME` ve `REBOOT` gibi tanılama komutları.

Bu noktada KBM gözle görülür şekilde “sistem” hissi vermeye başlar. Ama hâlâ user-mode OS olmak zorunda değildir.

### Milestone D — Depolama ve basit dosya sistemi

Amaç: kalıcı veriyle çalışmak.

1. Blok aygıtı kavramı ve okuma API'si.
2. RAM disk ile başla; gerçek diske hemen atlama.
3. Çok küçük, kendine ait read-only dosya sistemi veya basit FAT okuma.
4. Dosya listesi ve dosya içeriği görüntüleme.

Bu bölüm, aygıt sürücüsü, cache, hata yönetimi ve veri biçimi konularını birleştirir. Kapsamı küçük tutmak özellikle önemlidir.

### Milestone E — Süreçler ve kullanıcı modu (isteğe bağlı, ileri)

Amaç: kernel ile uygulamayı gerçekten ayırmak.

1. Ring 3 için GDT/IDT ve privilege kurallarını genişlet.
2. Ayrı kullanıcı sayfa tabloları kur.
3. Syscall girişi tasarla.
4. Minimal scheduler ve context switch yaz.
5. Küçük bir kullanıcı programı yükle.

Bu aşama çok öğreticidir ama önceki kilometre taşlarının sağlam olmasına bağlıdır. KBM'nin öğrenme amacı için zorunlu değildir.

## 4. Şimdiki sıra neden mantıklı?

Sıra bağımlılıklardan gelir:

~~~text
memory map
  → physical frames
    → mapped kernel memory
      → heap
        → dynamic drivers / console / filesystem structures
          → (istersen) processes and user mode
~~~

Timer ve interrupt altyapısı daha önce geldi çünkü ileride scheduler için temel olacak; ama şu an timerın varlığı schedulerın varlığı demek değildir. Benzer biçimde, GDT'nin varlığı ring 3 kullanıcı modu olduğu anlamına gelmez.

## 5. Başarıyı nasıl ölçmeli?

KBM'nin ilerlemesi satır sayısı veya Linux'a benzerlik ile ölçülmemeli. Daha iyi ölçütler:

- Bir özelliğin hangi problemi çözdüğünü anlatabiliyor musun?
- Hata olduğunda en yakın katmanı sistemli daraltabiliyor musun?
- QEMU'da küçük bir deneyle davranışı doğrulayabiliyor musun?
- Kodun sahiplik sınırları belirgin mi: “bu fiziksel frame kimin”, “bu interrupt kimden geldi”?
- Bir hafta sonra yorum ve manual sayesinde koda geri dönebiliyor musun?

Bu kernelin en değerli çıktısı boot eden ISO değil; CPU, bellek ve aygıtlar arasındaki neden-sonuç zincirini senin kurabilmen.

## 6. Önerilen çalışma ritmi

Her oturumda yalnızca bir küçük hedef seç:

~~~text
Bugün: USABLE aralıklarından hizalı ilk frame'i bul.
Sonra: o frame adresini yazdır.
Sonra: ikinci frame'in farklı ve hizalı olduğunu doğrula.
~~~

Her hedef için şu dört satırı bir not dosyasına yaz:

1. Hangi varsayımı test ediyorum?
2. Kaynakta hangi dosyalar bu işi yapıyor?
3. QEMU'da beklediğim kanıt ne?
4. Başarısız olursa önce hangi katmana bakacağım?

Bu ritim, karmaşık kernel çalışmasını yönetilebilir deneylere dönüştürür.

## 7. Bu projeyi nasıl konumlandırmalı?

KBM'nin hedefi Linux/Windows ile rekabet etmek değildir. Dürüst tanım şudur:

> Limine üzerinde boot eden; GDT, IDT, PIC/PIT timer, LAPIC MMIO eşlemesi, temel sayfalama incelemesi ve bellek haritası keşfi içeren, x86-64 için öğrenme odaklı bir hobby kernel.

Bu tanım hem yapılan işi küçümsemez hem de olmayan özellikleri varmış gibi göstermez. İyi teknik anlatımın temelidir.

## Son kontrol noktası

Şu anki en doğal heap görevi tamamen boş kalan bir heap frame'ini PMM'ye geri
vermektir. Ona başlamadan önce Bölüm 8, 9 ve 10'u bir kez daha oku; özellikle
şu cümleyi oturt:

> Sayfalama bir adrese erişebilme kuralıdır; allocator o adres arkasındaki RAM'i kullanma sahipliği kuralıdır.
