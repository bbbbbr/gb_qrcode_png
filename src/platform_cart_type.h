#ifndef PLATFORM_CART_TYPE_H
#define PLATFORM_CART_TYPE_H

// still need to do banked calls for duck

#if defined(CART_TYPE_md2)
    // .md2
    // ROM Upper switchable 16K banks (1-7)
    //
    // When used on MegaDuck Laptop, SRAM *may* be on a secondary cart in memory cart slot:
    // - Selected by writing (0 - 3) in Upper Nibble (mask 0x30)
    // - Bank Size/Region: 8k mapped at 0xA000 - 0xBFFF

    // Default GBDK ROM bank switch for MD2 is ok
    // #define PLAT_SWITCH_ROM(b) (_current_bank = (b), (*(volatile uint8_t *)0x0001 = b))
    #define PLAT_ENABLE_SRAM    // No need to, SRAM enabled by default
    #define PLAT_SWITCH_ROM(b) SWITCH_ROM(b)
    #define PLAT_SWITCH_RAM(b) (*(volatile uint8_t *)0x1000 = (b) << 4)
#elif defined(CART_TYPE_mbc5)
    // Forcing explicit MBC5 on Megaduck, also requires modified asn banked call trampolines (see src/duck_mbc5)
    #define PLAT_ENABLE_SRAM    (*(volatile uint8_t *)0x0000 = 0x0A)
    #define PLAT_SWITCH_ROM(b) (_current_bank = (b), (*(volatile uint8_t *)0x2000 = (b)))
    #define PLAT_SWITCH_RAM(b) (*(volatile uint8_t *)0x4000 = (b))
#else
    // MBC5
    #define PLAT_ENABLE_SRAM    ENABLE_RAM
    #define PLAT_SWITCH_ROM(b) SWITCH_ROM(b)
    #define PLAT_SWITCH_RAM(b) SWITCH_RAM(b)
#endif

#endif // PLATFORM_CART_TYPE_H