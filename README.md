# FX5 PLC c könyvtár

## Áttekintő
A projekt célja, hogy a Mitsubishi FX5 PLC-kel kommunikálni képes minimális C függvény könyvtárat hozzon létre, amit aztán mikrokontroller környezetből lehet használni.

## Követelmények

A könyvtárnak a következő funkcionalitással kell rendelkezzen:
- Egyetlen D regiszter írása
- Egyetlen D regiszter olvasása
- Egyetlen Y coil írása
- Egyetlen X coil olvasása
- Egyetlen M coil olvasása
- Egyetlen M coil írása
- Több D regiszer olvasása (opcionális)
- Több M coil olvasása (opcionális)

Kezelnie kell a soros és a TCP átviteli réteget is.
A könyvtár a PDU (protocol data unit) és a szállítási réteg (wire layer) szigorú szétválasztására épül. A PDU formátum minden átviteli mód esetén azonos.
A monitoring timer mező kizárólag a TCP (SLMP 3E) wire header része.

## Tervezési alapelvek
- Dinamikus memória használata nem megengedett
- Minden buffer-t a hívó fél biztosít
- A parser resync- képes (stream alapú feldolgozás)
- A válasz feldolgozása mindig a lekérdezés kontextusában történik
- A könyvtár nem thread-safe
- Limitáljuk az egyszerre írható/olvasható regiszterek számát (memória szükséglet alacsony tartása)
- Minden publikus függvényt és típust prefixelünk, a névtér ütközések elkerülése végett (FX5_ típusokra, és fx5_ függvényekre)
- A regiszterek átadása 16 bites tömbökben történik
- A bitek átadása is 16 bites tömbökben történik (0x00FF: true, 0x0000: false)
- A könyvtár két átviteli módot támogat TCP(SLMP 3E binary) és SERIAL (RS232/RS485 framing + BCC.
- A felhasználó felelőssége az átviteli mód beállítása, automatikus detektálás nincsen.


## Felépítés

A könyvtár az alábbi logikai modulokra bomlik:
- Lekérdezés generálása
- Segédfüggvények
- Válasz feldolgozása
- TCP (SLMP 3E) wire header generálás és feldolgozás
- RS232/RS485 wire header generálás és feldolgozás + BCC
- Tranzakció-kezelés és kontextus követés
- Magas szintű API



## Implementáció

Egy tranzakció a következő lépéseket jelenti:
- Hálózati beállítások definiálása (fx5_wire_settings_t)
- Lekérdezés generálása (fx5_request_t)
- Lekérdezés elküldése a PLC felé (a felhasználó feladata)
- Válasz fogadása a PLC felől (a felhasználó feladata)
- Válasz feldolgozása (fx5_response_t)

Ennek megfelelően épül fel az fx5_transaction_t struktúra.
A publikus API-t az fx5.h fájl tartalmazza.

