# Sprint Log

Project work began on 2 November 2026, as confirmed by Khaled. The log records project activities and their available dates.

| Date | Event |
|---|---|
| 2026-11-02 | Project work began |
| 2026-11-03 | Call between Khaled and Desmond via Google Meet |
| 2026-11-10 | Call between Desmond and Khaled (Discord, ~8 min): simulated sensors, GitHub, Desmond Scrum Master; project plan v1 sent by e-mail |
| 2026-11-11 | Team call via Discord |
| 2026-11-11 | Sebastian approves simulation **if there is a HAL** by e-mail; asks for a complete, demonstrable prototype; demo as recorded customer video; deadline 27 Nov |
| 2026-11-11 | Khaled reviews plan v1 (8 points: HAL, lost data, Wokwi/MQTT, priorities, tests, sensor sources, work split, schedule/security); finds the individual reflection requirement (e-mail, Discord) |
| 2026-11-12 | Proposed work split on Discord: Khaled testing, Desmond simulation |
| 2026-11-14 | Team call via Discord |
| 2026-11-15 | Team call via Discord |
| 2026-11-19 | Team call via Discord |
| 2026-11-24 | Call with the whole team via Discord |

Teamet har även löpande skriftlig kommunikation via Discord.

## Sprint 1: plan and core

Goal: approved plan, HAL-based firmware, data path to an external system.
Done: plan v1 and v2, HAL and simulation, MQTT with buffering, gateway with history and warnings,
44 automated tests (25 firmware, 19 gateway), documentation. The work began on 2 November; the exact sprint boundaries need confirmation.

## Sprint 2: verify and demonstrate

Goal: Wokwi tests T-13 – T-18, update the test report with real results, demo script and recording.

Local testing update, 19 Nov: Khaled ran T-13–T-17 in Wokwi and T-18 on Windows, added alarm boundary tests and recorded the results in `docs/05-test-report.md`. The Windows rerun passed 30/30 firmware and 19/19 gateway tests after fixing the Unity test setup. T-16 passed the controlled scenarios described in the report; their limitations remain. Gayathri's separate T-18 run and the video recording are not verified by these results.

## Buffer (23 – 27 Nov)

Fixes after feedback, individual reflection reports, final hand-in.

## Sprint retrospective

Underlag för teamets granskning, baserat på projektloggen och de dokumenterade testkörningarna.

### Det som fungerade bra

- Arbetsfördelningen gjorde att planering, implementation och testning kunde följas upp i gemensam dokumentation.
- Simuleringen i Wokwi gjorde det möjligt att testa mätvärden, sensorfel, larm och lagring utan fysiska enheter.
- Testresultat och avvikelser dokumenterades i [testrapporten](05-test-report.md). Efter rättning av testuppsättningen klarade Windows-körningen 30 av 30 firmwaretester och 19 av 19 gatewaytester.
- Kontrollerade frånkopplingar gav underlag för att granska buffring och återanslutning. T-16-resultaten och deras begränsningar dokumenterades.

### Det som var svårt

- En utgången Wokwi-licens blockerade först de lokala simulatortesterna; efter aktivering kunde testerna genomföras.
- Unitys testuppsättning saknade nödvändiga funktioner i fem testsviter, vilket orsakade länkningsfel på Windows innan uppsättningen rättades.
- Återanslutningstesternas resultat täcker de kontrollerade scenarierna, men verifierar inte alla typer av nätverksfel.

### Föreslagna förbättringar

- Kontrollera licenser, beroenden och testmiljöer på varje dator tidigt i arbetet.
- Kör tester på de operativsystem som används av teamet och dokumentera både felorsak och omkörning.
- Bestäm testscenarier och begränsningar i förväg, särskilt för frånkoppling, buffring och återanslutning.
- Samla status, testresultat och underlag löpande i gemensam dokumentation inför granskning och presentation.
