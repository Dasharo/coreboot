# Zadanie: cofnięcie descriptora x2 NVMe dla Protectli V1000 (JSL) w dasharo-blobs

Pracujesz w repo `dasharo-blobs`. Odpowiadaj po polsku, commity pisz po angielsku.

## Kontekst
- Commit `cc5549a` ("protectli/vault_jsl: Update descriptor for x2 NVMe links", Michał Kopeć, 2026-05-12) zmienił `descriptor.bin` dla V1210, V1410 i V1610 (FD v1.3 / v1.4 dla V1410 wg README):
  lane reversal na SPA, bifurkacja 2x2 lub 1x2+2x1, SSD na x2.
- Lane reversal zamienia kolejność root portów. Na V1410 zamienia to LAN1 i LAN2, także w pfSense/FreeBSD
  (nazywa interfejsy wg devfn, więc nie da się tego załatać w coreboocie). Na V1610 przesuwa numery busów o +1.
- Klient Protectli zdecydował: **wracamy do x1 dla całej serii V1000**, bez modyfikacji FSP.
- Commit `8a20db6` ("Disable SATA in the descriptor", 2026-04-14) to ostatni stan x1. Zweryfikowano, że `cc5549a~1` == `8a20db6`,
  a `descriptor.bin` dla V1210/V1410/V1610 są w obu identyczne.
- Po stronie coreboota rewert jest gotowy (branch `protectli-jsl-revert-x2`, commit `63335db4fd`). Brakuje tylko blobów.

## Do zrobienia
1. `git status` i `git log --oneline -5`. Zanotuj, na jakim jesteś branchu i commicie. Jeśli drzewo jest brudne, zatrzymaj się i zapytaj.
2. Utwórz branch `protectli-jsl-revert-x2` z bieżącego HEAD. Nie pracuj na głównym branchu.
3. `git revert cc5549a`. Jeśli są konflikty (np. w README po późniejszych zmianach), rozwiąż je zachowując późniejsze zmiany
   niezwiązane z V1x10. Pokaż mi konflikty, zanim je rozstrzygniesz w niejednoznacznym miejscu.
4. Zweryfikuj wynik:
   - `descriptor.bin` w `protectli/vault_jsl/{V1210,V1410,V1610}` jest bajt w bajt taki jak w `8a20db6`
     (`git diff --quiet 8a20db6 HEAD -- protectli/vault_jsl/V1210/descriptor.bin` itd.).
   - Z README zniknęły wpisy changelogu V1.3 (V1.4 dla V1410), a nagłówki wersji wracają do v1.2 (v1.3 dla V1410).
   - `cd protectli/vault_jsl && sha256sum -c SHA256SUMS` przechodzi.
   - Rewert odtwarza `SHA256SUMS.asc` i `SHA256SUMS.sig` z `8a20db6`, czyli już podpisane przez Michała.
     Sprawdź, jak repo weryfikuje podpisy (README w głównym katalogu lub `protectli/vault_jsl/README.md`), i zweryfikuj, jeśli to możliwe.
     Jeśli się nie da, napisz to wprost zamiast zakładać, że jest OK.
5. Commit: zostaw domyślną wiadomość rewertu i dopisz uzasadnienie (lane reversal swaps LAN1/LAN2 on V1410 in pfSense/FreeBSD,
   shifts bus numbers on V1610, customer chose x1 for the whole V1000 series). Dodaj `Signed-off-by` i na końcu:
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
   Git nie ma tu tożsamości, więc użyj `git -c user.name="Wiktor Mowinski" -c user.email="wiktor.mowinski@3mdeb.com" commit -s ...`.
   Nie zmieniaj globalnej konfiguracji gita.
6. Wypisz mi: hash nowego commita, wynik `git diff --stat 8a20db6 HEAD -- protectli/vault_jsl` (powinien być pusty dla `*.bin`),
   wynik weryfikacji sum i podpisów.

## Czego NIE robić
- Nie pushuj niczego i nie otwieraj PR. Push do `Dasharo/dasharo-blobs` uzgadniam osobno z Michałem.
- Nie ruszaj innych platform (`vault_adl_n`, `vault_adl_p`, `vault_cml`, `vault_ehl`, `vault_glk`, `vault_bsw`) ani innych vendorów.
- Nie generuj ani nie podpisuj sum własnoręcznie i nie edytuj `descriptor.bin`. Ma pochodzić wyłącznie z rewertu.
- Nie używaj `--force`, `reset --hard` ani `rebase` na istniejących branchach.

## Po stronie coreboota (informacyjnie, tego tu nie rób)
Po wypchnięciu blobów trzeba w coreboocie podbić wskaźnik submodułu `3rdparty/dasharo-blobs` na nowy commit.
Bump zrobię osobno, po ustaleniu z Michałem, co jest wypchnięte.
