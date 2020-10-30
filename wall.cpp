#include "wall.hpp"


struct iovec g_remote[1024], g_local[1024];



struct wall::GlowObjectDefinition_t g_glow[1024];



int cachedSpottedAddress = -1;
int count = 0;


void wall::Glow(remotecall::Handle* csgo, remotecall::MapModuleMemoryRegion* client, unsigned long glowAddress) {
    if (!csgo || !client)
        return;

    bzero(g_remote, sizeof(g_remote));
    bzero(g_local, sizeof(g_local));
    bzero(g_glow, sizeof(g_glow));

    wall::CGlowObjectManager manager;

    if (!csgo->Read((void*) glowAddress, &manager, sizeof(wall::CGlowObjectManager))) {
        // std::cout << "Failed to read glowClassAddress" << std::endl;
        return;
    }

    size_t count = manager.m_GlowObjectDefinitions.Count;

    void* data_ptr = (void*) manager.m_GlowObjectDefinitions.DataPtr;

    if (!csgo->Read(data_ptr, g_glow, sizeof(wall::GlowObjectDefinition_t) * count)) {
        // std::cout << "Failed to read m_GlowObjectDefinitions" << std::endl;
        return;
    }

    size_t writeCount = 0;

    for (unsigned int i = 0; i < count; i++) {

        if (g_glow[i].m_pEntity != NULL) {
              wall::Entity ent;
            if (csgo->Read(g_glow[i].m_pEntity, &ent, sizeof(wall::Entity))) {
                if (ent.m_iTeamNum != 2 && ent.m_iTeamNum != 3 ||
                    ent.m_isDormant == 1) {
                    g_glow[i].m_bRenderWhenOccluded = 0;
                    g_glow[i].m_bRenderWhenUnoccluded = 0;
                    continue;
                }
                if(g_glow[i].m_bRenderWhenOccluded == 1) {
                  continue;
                }
                
                g_glow[i].m_bRenderWhenOccluded = 1;
                g_glow[i].m_bRenderWhenUnoccluded = 0;

                if (ent.m_iTeamNum == 2) {
                    g_glow[i].m_flGlowRed = 1.0f;
                    g_glow[i].m_flGlowGreen = 0.0f;
                    g_glow[i].m_flGlowBlue = 0.0f;
                    g_glow[i].m_flGlowAlpha = 0.55f;

                } else if (ent.m_iTeamNum == 3) {
                    g_glow[i].m_flGlowRed = 0.0f;
                    g_glow[i].m_flGlowGreen = 0.0f;
                    g_glow[i].m_flGlowBlue = 1.0f;
                    g_glow[i].m_flGlowAlpha = 0.55f;
                }
            }
        }

        size_t bytesToCutOffEnd = sizeof(wall::GlowObjectDefinition_t) - g_glow[i].writeEnd();
        size_t bytesToCutOffBegin = (size_t) g_glow[i].writeStart();
        size_t totalWriteSize = (sizeof(wall::GlowObjectDefinition_t) - (bytesToCutOffBegin + bytesToCutOffEnd));
        g_remote[writeCount].iov_base =
                ((uint8_t*) data_ptr + (sizeof(wall::GlowObjectDefinition_t) * i)) + bytesToCutOffBegin;
        g_local[writeCount].iov_base = ((uint8_t*) &g_glow[i]) + bytesToCutOffBegin;
        g_remote[writeCount].iov_len = g_local[writeCount].iov_len = totalWriteSize;
        writeCount++;
         
    }
    process_vm_writev(csgo->GetPid(), g_local, writeCount, g_remote, writeCount, 0);

}