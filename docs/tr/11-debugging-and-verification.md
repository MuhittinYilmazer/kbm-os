# 11 — Hata ayıklama ve doğrulama

Kernel hata ayıklaması normal bir kullanıcı programından daha zordur: çoğu zaman dosyaya log yazamazsın, ekran sürücün tam değildir ve bir hata doğrudan CPU'yu durdurabilir. Bu bölümde KBM'nin mevcut gözlemlenebilir araçlarını sistemli kullanıyoruz.

## 1. Önce hangi aşamada kaldığını bul

Kernelin seri çıktıdaki işaretçileri tesadüfi `printf`ler değildir. Her biri önyükleme zincirindeki bir sınırı doğrular. Tipik sıra:

~~~text
KBM_BOOT_OK
KBM_HHDM_READY
KBM_CR3: ...
KBM_GDT_LOADER_CALLED
KBM_IDT_LOADED
KBM_HHDM_REQUEST_PHYS: ...
KBM_PML4_511: ...
KBM_PDPT_510: ...
KBM_PD_0: ...
KBM_PT_0: ...
KBM_LAPIC_MMIO_MAPPED
KBM_PIC_REMAP_OK
KBM_PIT_TICKS: ...
KBM_MEMMAP_ENTRIES: ...
~~~

En son görünen işaretçi, hata aramasını daraltır:

| Son görünen durum | İlk bakılacak yer |
| --- | --- |
| Hiç seri çıktı yok | ISO/boot config, seri sürücü, `kmain`e ulaşma |
| `BOOT_OK` var, HHDM yok | Limine HHDM cevabı ve null kontrolü |
| GDT'den hemen sonra duruyor | `gdt_load.S`, selector değerleri, stack |
| IDT'den sonra duruyor | IDT gate kurulumu, assembly stub, beklenmeyen exception |
| LAPIC eşleme öncesi duruyor | CR3 page-table yürüyüşü, HHDM fiziksel/sanal ayrımı |
| `PIC_REMAP_OK` var ama tick yok | PIC maskesi, PIT programlama, IF, LINT0, EOI |
| Tick var, memory map yok | Tick bekleme döngüsü sonrası `main.c` |

Bu tablo kesin teşhis değildir; doğru katmana ilk adımı attıran bir pusuladır.

## 2. Güvenilir çalışma komutları

Önce derle:

~~~sh
make all
~~~

Grafik penceresiyle başlatmak için:

~~~sh
make run
~~~

Seri logu terminalde görmek için mevcut ISO'yu doğrudan QEMU ile çalıştır:

~~~sh
qemu-system-x86_64 -M q35 -cdrom kbm.iso -boot d -m 2G \
  -display none -serial stdio -monitor none -no-reboot
~~~

Bu komuttaki seçeneklerin amacı:

| Seçenek | Etkisi |
| --- | --- |
| `-M q35` | Modern PC benzeri Q35 platformunu seçer. |
| `-cdrom kbm.iso -boot d` | ISO'yu CD olarak takar ve ondan boot eder. |
| `-m 2G` | Konuk makineye 2 GiB RAM verir. |
| `-display none` | Grafik pencere açmaz. |
| `-serial stdio` | COM1 verisini terminale bağlar. |
| `-monitor none` | Terminali QEMU monitorü ile paylaşmaz. |
| `-no-reboot` | Hata sonrası sürekli yeniden başlayıp logu kaybettirmez. |

`make run` ekran/graphical framebuffer denemeleri için uygundur. Seri komutu ise “son işaretçi neydi?” sorusu için daha pratiktir.

## 3. Değişiklik yaparken küçük deney kuralı

Bir anda GDT, IDT, PIT ve sayfalama kodunu değiştirme. Kernelde bu, hangi katmanın bozulduğunu anlaşılmaz yapar.

Sağlıklı döngü şudur:

1. Tek bir hipotez yaz: örneğin “IRQ0 maskesini açınca timer gelecektir.”
2. En yakın iki noktaya işaretçi koy: çağrıdan önce ve sonra.
3. Derle.
4. QEMU'da seri çıktıyı karşılaştır.
5. Sonucu not et; sonra bir sonraki küçük değişikliğe geç.

İşaretçiler kalıcı bir tasarım aracı değildir. Bir katman güvenilir hale geldiğinde gürültülü geçici logu kaldırabilir veya anlamlı bir boot aşaması loguna dönüştürebilirsin.

## 4. Güvenli exception deneyi

`main.c` içinde yorum olarak bırakılmış `ud2` testi varsa, **geçici olarak** açmak CPU'nun invalid-opcode exception'ını üretir. `ud2`, özellikle geçersiz opcode oluşturmak için tanımlanmış bir x86 talimatıdır.

Beklenen akış:

~~~text
CPU ud2 yürütür
→ vector 6 (#UD)
→ IDT[6]
→ isr_invalid_opcode
→ exception_fatal(6)
→ seri porta hata mesajı
→ CPU durur
~~~

Bu test, IDT'nin yalnızca “kurulmuş” görünmediğini gerçekten exception yakaladığını gösterir. Testi bitirince tekrar yorum satırına al; aksi halde timer ve sonraki aşamalara ulaşamazsın.

Benzer biçimde, yalnızca laboratuvar ortamında `int $32` yazılım interrupt'ı ile vector 32 yolunu test edebilirsin. Donanım IRQ'sundan farklı olarak bu CPU'ya doğrudan “bu vector'e git” dersin. EOI gerektirmeyen yapay test ile gerçek PIT IRQ'sunu birbirine karıştırma.

## 5. Yaygın hatalar ve nedenleri

### “IDT yükledim ama `sti`den sonra kilitleniyor”

Olasılıklar:

- IDT entry'sinin handler adresi veya selector'ı yanlış.
- Gate type/present alanı yanlış.
- IRQ geldiğinde stub'un stack dengesini bozuyor.
- PIC beklenmeyen bir vector'e yönlendiriyor.
- Exception handler `iretq` ile dönmesi gerektiği halde dönmüyor ya da fatal durumda durması gerekirken dönmeye çalışıyor.

Önce exception mı IRQ mı geldiğini ayır: exception handler seri porttan vector numarasını yazıyor. Timer handler ise `timer_irq`e gider.

### “PIT sayacı artmıyor”

PIT tek başına yeterli değildir. Yolun tamamı gerekir:

~~~text
PIT → PIC IRQ0 → LAPIC LINT0 → CPU interrupt kabulü → IDT[32] → ISR → timer_irq → PIC EOI
~~~

Kontrol listesi:

- `pit_init()` doğru control word ve divisor yazıyor mu?
- `pic_remap()`ten sonra IRQ0 unmask edildi mi?
- `sti` gerçekten çağrıldı mı? RFLAGS içindeki IF biti açık mı?
- LAPIC LINT0 maskeli mi veya ExtINT yerine yanlış delivery mode'da mı?
- `timer_irq()` master PIC'e EOI gönderiyor mu?
- `hlt` döngüsü interrupt açıkken mi çalışıyor?

Bu proje, bu zincirin LINT0 halkasını özellikle elle kurmak zorunda kaldı. Bu bir bootloader hatası değil: bootloader CPU/devletini kernel için makul bir başlangıca getirir, fakat kernelin kendi interrupt yönlendirmesini güvenilir biçimde sahiplenmesi gerekir.

### “Sayfa hatası aldım veya LAPIC'e erişince durdu”

En sık hata fiziksel adresi sanal adres sanmaktır. LAPIC'in fiziksel adresi `0xFEE00000` doğrudan C pointer'ı olarak kullanılamaz; o fiziksel sayfa önce bir sanal sayfaya PTE ile map edilmelidir.

Şunları tek tek doğrula:

- CR3'ten alınan PML4 fiziksel adresini HHDM ile erişilebilir pointer'a çeviriyor musun?
- `paging_translate_4k` çıktısı gerçekten fiziksel adres mi?
- Boş PML4/PDPT/PD/PT entry'lerini uygun present/writable bitleriyle mi kurdun?
- PTE'de LAPIC fiziksel frame'i ve cache-control bitleri var mı?
- `invlpg` sonrasında mı erişiyorsun?

## 6. Assembly/C sınırını ayırarak düşün

C fonksiyonuna girmek normal bir çağrı sözleşmesi ister; hardware interrupt ise CPU'nun oluşturduğu farklı bir stack frame ile gelir. Bu nedenle `interrupts.S` kodu “gereksiz assembly” değildir. Köprü görevi yapar:

~~~text
CPU interrupt frame
→ register save
→ 16-byte stack alignment
→ C handler call
→ register restore
→ iretq
~~~

Bir ISR hatasında önce şu soruyu sor: “Sorun C mantığında mı, yoksa C'ye girmeden önce/çıktıktan sonra stack sözleşmesinde mi?” Bu ayırım çok zaman kazandırır.

## 7. GDB: sonraki seviye araç

Seri log yeterli olmadığında QEMU, GDB ile duraklatılabilir. Tipik yaklaşım QEMU'yu debug portu açık ve CPU başlangıçta durmuş şekilde başlatmaktır:

~~~sh
qemu-system-x86_64 ... -s -S
~~~

Başka bir terminalden:

~~~sh
gdb kernel/bin/kernel
(gdb) target remote :1234
(gdb) break kmain
(gdb) continue
~~~

`-S`, CPU'yu GDB devam komutunu verene kadar durdurur; `-s` varsayılan olarak TCP 1234'te GDB stub açar. Bu akış yararlı olsa da ilk sorun arama aracın log ve küçük deneyler olmalıdır. GDB, assembly, optimizasyon ve sembol/adres ayrıntıları nedeniyle başlangıçta ağır gelebilir.

## 8. LAPIC/LINT0 hata ayıklama günlüğünden çıkarılan ders

Bu projede PIT kodu, PIC remap ve IDT çalışıyor gibi görünürken tick gelmemişti. Sorun “PIT QEMU'ya özel ve bozuk” değildi. PIC'in legacy interrupt çıkışı LAPIC'in **LINT0** girişinden CPU'ya ulaşır; LINT0 maskeli ExtINT durumundaydı.

Çözümün mantığı:

1. LAPIC fiziksel register sayfasını özel bir sanal adrese map et.
2. LINT0 Local Vector Table kaydını oku.
3. Delivery mode'u ExtINT olarak ayarla.
4. Mask bitini temizle.
5. PIC IRQ0, vector 32 olarak IDT'ye ulaşınca `timer_irq` sayacı artırır.

Buradaki esas kazanım tek tek register bitlerini ezberlemek değil, şu yöntemi öğrenmektir: **bir donanım sinyalini kaynaktan handler'a kadar tüm halkalarıyla takip et.**

## Kontrol noktası

- Son boot işaretçisi aramayı hangi dosya/katmana daralttığını söyler.
- `ud2` vector 6 için kontrollü bir exception testidir.
- Tick sorunu PIT'ten IDT'ye uzanan zincirin herhangi bir halkası olabilir.
- GDB bir sonraki seviye araçtır; seri log ilk gözlem aracıdır.
