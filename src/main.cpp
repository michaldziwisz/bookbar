// main.cpp — deklaracja komponentu foobar2000 Bookbar.
// Bookbar: DSP do audiobookow (tempo/wysokosc/rate + wzbogacanie dzwieku),
// port wtyczki Winamp "Bookamp". C++/SDK foobar2000, x64. Licencja: GPLv3.
#include <SDK/foobar2000.h>

#define BOOKBAR_VERSION "1.1.0"

DECLARE_COMPONENT_VERSION(
    "Bookbar",
    BOOKBAR_VERSION,
    "Bookbar " BOOKBAR_VERSION " - audiobook DSP (tempo, pitch, tape speed + sound enhancement).\n"
    "Author: Michal Dziwisz. Consultant: Patryk Faliszewski.\n"
    "Accessible (NVDA/JAWS). GPLv3. https://github.com/michaldziwisz/bookbar\n"
    "Port of the Winamp 'Bookamp' plugin."
);

// Zapobiega zmianie nazwy pliku komponentu (poprawne dzialanie troubleshootera).
VALIDATE_COMPONENT_FILENAME("foo_bookbar.dll");
