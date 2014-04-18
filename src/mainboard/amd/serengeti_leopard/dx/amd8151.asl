// AMD8151 
            Device (AGPB)
            {
                Name (_ADR, 0x00020000)
                Name (APIC, Package (0x04)
                {
                    Package (0x04) { 0x0000FFFF, 0x00, 0x00, 0x10 }, 
                    Package (0x04) { 0x0000FFFF, 0x01, 0x00, 0x11 }, 
                    Package (0x04) { 0x0000FFFF, 0x02, 0x00, 0x12 }, 
                    Package (0x04) { 0x0000FFFF, 0x03, 0x00, 0x13 }
                })
                Name (PICM, Package (0x04)
                {
                    Package (0x04) { 0x0000FFFF, 0x00, \_SB.PCI1.LNKA, 0x00 }, 
                    Package (0x04) { 0x0000FFFF, 0x01, \_SB.PCI1.LNKB, 0x00 }, 
                    Package (0x04) { 0x0000FFFF, 0x02, \_SB.PCI1.LNKC, 0x00 }, 
                    Package (0x04) { 0x0000FFFF, 0x03, \_SB.PCI1.LNKD, 0x00 }
                })
                Method (_PRT, 0, NotSerialized)
                {
                    If (LNot (PICF)) { Return (PICM) }
                    Else { Return (APIC) }
                }
            }

