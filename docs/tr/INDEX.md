# KBM El Kitabı

Bu el kitabı, KBM'nin mevcut x86-64 kernel kodunu anlamak için hazırlanmıştır.
Okuma sırası kasıtlıdır: önce makinenin ve boot zincirinin büyük resmi, sonra
CPU tabloları, interrupt'lar ve bellek gelir.

1. [El kitabını okuma biçimi](00-reading-the-manual.md)
2. [Proje özeti ve kaynak ağacı](01-project-overview-and-source-tree.md)
3. [Güç düğmesinden `kmain()`e](02-from-power-on-to-kmain.md)
4. [KBM'yi derlemek ve QEMU'da çalıştırmak](03-building-and-running-kbm.md)
5. [Freestanding C, çıktı ve ölümcül hatalar](04-freestanding-c-output-and-failures.md)
6. [CPU durumu, GDT ve stack](05-cpu-state-gdt-and-stack.md)
7. [IDT, exception ve `iretq`](06-idt-exceptions-and-iretq.md)
8. [PIC, PIT ve timer IRQ](07-pic-pit-and-the-timer-irq.md)
9. [Fiziksel bellek, sanal bellek ve paging](08-physical-memory-virtual-memory-and-paging.md)
10. [LAPIC, LINT0 ve MMIO](09-lapic-lint0-and-mmio.md)
11. [Limine memory map ve sonraki bellek katmanı](10-limine-memory-map-and-next-memory-layer.md)
12. [QEMU ile hata ayıklama ve doğrulama](11-debugging-and-verification.md)
13. [Mevcut durum, sınırlar ve yol haritası](12-current-status-limitations-and-roadmap.md)
14. [PMM, heap, console, klavye ve shell](13-memory-allocator-console-keyboard-and-shell.md)
15. [Sözlük](appendix-glossary.md)

İngilizce sürüm: [English manual](../en/INDEX.md)
