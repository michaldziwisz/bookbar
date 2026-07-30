// engine_factory.cpp — fabryka silnikow.
#include "engine.h"

namespace bookbar {

IStretchEngine *createSoundTouchEngine();
IStretchEngine *createBungeeEngine();

IStretchEngine *createEngine(Algo a) {
    switch (a) {
    case ALGO_SOUNDTOUCH: return createSoundTouchEngine();
    case ALGO_BUNGEE:     return createBungeeEngine();
    default:              return nullptr;
    }
}

} // namespace bookbar
