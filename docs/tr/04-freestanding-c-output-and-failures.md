# Freestanding C, çıktı ve ölümcül hatalar

Normal bir Linux C programı printf, malloc, libc başlangıç kodu ve işletim
sisteminin sunduğu dosya veya terminal hizmetlerini hazır bulur. Kernel böyle
bir ortamda çalışmaz. KBM, bu yüzden en temel ihtiyaçlarını kendisi sağlar.

## Freestanding ne demektir?

Freestanding bir C ortamında derleyici C dilini bilir, fakat bir işletim
sistemi ve standart C kütüphanesi bulunduğunu varsaymaz. Örneğin derleyici
optimizasyon sırasında memcpy, memset, memmove veya memcmp çağrısı üretebilir.
kernel/src/memory.c, bu fonksiyonların KBM sürümlerini sağlar.

Bu dosyanın varlık sebebi her kodu elle yazmak değildir. Derleyicinin ürettiği
temel bellek işlemlerinin çözülmesini sağlamaktır.

## COM1 seri portu

Kernel erken aşamada ekrana güvenemez. Framebuffer response gelmemiş olabilir,
ekran sürücüsü bozuk olabilir veya page fault ekrana yazmadan önce oluşabilir.
Bu nedenle KBM ilk olarak serial_init çağırır.

drivers/serial.c, geleneksel ilk UART olan COM1'i kullanır:

~~~
COM1 I/O port tabanı: 0x3F8
~~~

outb ve inb normal RAM pointer'ı kullanmaz. Bunlar x86 I/O port alanına byte
yazar veya okur. serial_putc, UART'ın iletim kaydının boşalmasını bekler;
serial_write metni karakter karakter gönderir. serial_write_hex adres ve bit
maskelerini sabit genişlikte hexadecimal yazdırır.

## Framebuffer çizimi

Limine framebuffer response sağlarsa drivers/framebuffer.c pixel belleğine
yazar. Framebuffer, görüntü donanımının okuduğu bir bellek bölgesidir.

~~~
screen_put_pixel  -> tek pixel
screen_fill       -> bütün ekran
screen_draw_rect  -> dikdörtgen
screen_draw_glyph -> 8x8 bitmap karakter
~~~

pitch, bir satırdaki byte sayısıdır; her zaman width * 4 olmak zorunda değildir
çünkü donanım satır sonunda padding bırakabilir. Pixel pointer'ının volatile
olması, görüntü belleğine yapılan yazıların derleyici tarafından atlanmamasını
sağlar.

Bu fonksiyonlar şu anda main.c içinde yorum satırındadır. Yani framebuffer
altyapısı derlenir, ancak mevcut normal boot yolunda aktif bir çizim yapılmaz.

## Panic ve durma

Kernel güvenli devam edemeyecek bir durum görürse panic çağırır. Örneğin Limine
HHDM veya memory-map response'u yoksa devam etmek bellek erişimlerini anlamsız
hale getirir.

~~~
panic(message)
  -> KBM PANIC: ... mesajını COM1'e yazar
  -> cpu_halt_forever çağırır
~~~

cpu_halt_forever önce cli ile maskelenebilir interrupt'ları kapatır, sonra
sonsuz hlt döngüsüne girer. Bu, hatalı kernelin rastgele kod çalıştırmaya devam
etmesini engeller.

## Kontrol noktası

1. Freestanding kernel neden host libc'ye güvenemez?
2. outb ile normal pointer yazısı arasındaki fark nedir?
3. Framebuffer pointer'ı neden volatile olabilir?
4. Panic sonrasında kernel neden normal C dönüşü yapmaz?
