# El kitabını okuma biçimi

KBM, Linux veya Windows ile rekabet etmeyi amaçlayan bir ürün değildir. C ve
x86-64 assembly kullanarak kernel geliştirmeyi öğrenmek için yazılan bir hobi
kernelidir. Bu yüzden kodun bazı bölümleri küçük, kasıtlı olarak doğrudan ve
öğretici niteliktedir.

## Bu el kitabının kapsamı

Metin yalnızca depodaki mevcut davranışı anlatır. KBM timer interrupt alır,
LAPIC'e MMIO üzerinden erişir, Limine memory map'iyle frame allocator kurar,
bump heap kullanır ve framebuffer console + PS/2 klavye shell'i çalıştırır.
Filesystem, process, kullanıcı modu ve kendi bootloader'ı henüz yoktur. Bunlar
gelecek hedef olarak belirtilir; yapılmış özellik gibi anlatılmaz.

## Okuma stratejisi

Kernel kodunda iki soru sürekli sorulmalıdır:

```text
Bu veri nerede yaşıyor?
Bu noktada kontrol kimde?
```

Bir değer CPU register'ında, stack'te, normal RAM'de, page table içinde veya
bir cihaz kaydında olabilir. Kontrol ise normal C kodunda, CPU'nun exception
giriş yolunda, bir interrupt handler'ında, Limine'de veya firmware'de olabilir.
Bu iki soruyu sormak, terim ezberlemekten daha faydalıdır.

## Kodun ve manualın rolleri

Kaynak kod, gerçek davranışın otoritesidir. Manual bir fonksiyonun tamamını
tekrar basmak yerine şu soruları cevaplar:

- Bu dosya hangi problemi çözüyor?
- CPU veya cihaz hangi sırayla hareket ediyor?
- Adres fiziksel mi, sanal mı, cihaz adresi mi?
- Hangi QEMU çıktısı bu davranışı kanıtlıyor?
- Bu kodun bugünkü sınırı nedir?

Her bölümün sonunda yer alan kontrol soruları sınav değildir. Amaç, okuyucunun
ekrana bakmadan kısa bir çalışma hikâyesi kurabilmesidir.

## Ön koşullar

Temel C sözdizimi, fonksiyon, pointer ve bit işlemlerini bilmek yeterlidir.
Assembly, BIOS, UEFI, page table veya interrupt bilgisi varsayılmaz; ilgili
bölümde ilk kullanıldıklarında açıklanırlar.

## İlk zihinsel model

```text
Firmware -> Limine -> KBM kmain()
                         |
                         +-> CPU tabloları
                         +-> çıktı sürücüleri
                         +-> interrupt altyapısı
                         +-> bellek altyapısı
```

İlk kez okuyorsanız bölümleri sırayla okuyun. Belirli bir hata araştırıyorsanız
önce 11. bölüme, ardından ilgili alt sistem bölümüne dönün.

## Kontrol noktası

1. KBM'nin bugünkü amacı neden "tam işletim sistemi" değildir?
2. Kernel kodunu incelerken sorulacak iki temel soru nedir?
3. Manual ile kaynak kod arasındaki otorite ilişkisi nedir?
