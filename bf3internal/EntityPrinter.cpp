#include "EntityPrinter.h"
#include "FB SDK/Frostbite.h"
#include <Windows.h>
#include <cstdio>



// we pray to chatgpt 🙏
namespace EntityPrinter {
    void PrintAllEntities() {
        OutputDebugStringA("[EntityPrinter] Starting entity print...\n");
        
        fb::ClientGameContext* ctx = fb::ClientGameContext::Singleton();
        if (!ctx) {
            OutputDebugStringA("[EntityPrinter] No game context!\n");
            return;
        }
        OutputDebugStringA("[EntityPrinter] Game context found\n");
        
        if (!ctx->m_level) {
            OutputDebugStringA("[EntityPrinter] No level!\n");
            return;
        }
        OutputDebugStringA("[EntityPrinter] Level found\n");
        
        fb::ClientLevel* level = ctx->m_level;
        if (!level->m_gameWorld) {
            OutputDebugStringA("[EntityPrinter] No game world!\n");
            return;
        }
        OutputDebugStringA("[EntityPrinter] Game world found\n");
        
        fb::GameWorld* gw = level->m_gameWorld;
        char buf[256];
        int entityCount = 0;
        
        snprintf(buf, sizeof(buf), "[EntityPrinter] Collections size: %llu\n", (unsigned long long)gw->m_collections.size());
        OutputDebugStringA(buf);
        
        for (size_t c = 0; c < gw->m_collections.size(); ++c) {
            fb::EntityWorld::EntityCollection& collection = gw->m_collections.at(c);
            snprintf(buf, sizeof(buf), "[EntityPrinter] Collection %llu: firstSegment=%p\n", (unsigned long long)c, collection.firstSegment);
            OutputDebugStringA(buf);
            
            fb::EntityCollectionSegment* seg = collection.firstSegment;
            while (seg) {
                snprintf(buf, sizeof(buf), "[EntityPrinter] Segment: m_Collection.size()=%llu\n", (unsigned long long)seg->m_Collection.size());
                OutputDebugStringA(buf);
                
                for (size_t i = 0; i < seg->m_Collection.size(); ++i) {
                    fb::Entity* ent = seg->m_Collection[i];
                    if (ent) {
                        snprintf(buf, sizeof(buf), "[EntityPrinter] Entity: %p, Flags: 0x%08X, WeakToken: 0x%08X\n", ent, ent->m_flags, ent->m_weakTokenHolder);
                        OutputDebugStringA(buf);
                        ++entityCount;
                    }
                }
                seg = seg->m_next;
            }
        }
        snprintf(buf, sizeof(buf), "[EntityPrinter] Total entities: %d\n", entityCount);
        OutputDebugStringA(buf);
    }
} 