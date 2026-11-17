# Demomanus: kundvideo för HydroGreen Fingers (ca 6–7 min)

Sebastian vill ha en inspelad demo **för kund**: ingen kod, fokus på vad systemet gör och varför.
Spela in med Google Meet eller OBS. Varje person pratar om sin egen del.

## Förberedelser (10 min innan)
1. `pio run -e wokwi` och starta Wokwi-simulatorn. Kontrollera att den gröna lysdioden lyser.
2. Starta mottagaren i en terminal: `python -m gateway.receiver --mqtt-host broker.hivemq.com --device mh-grupp6 --db data/demo.db`
3. Ha `docs/03-architecture.md` (diagrammet) och en tidigare genererad graf öppna.

## Manus

| Tid | Vem | Visa | Säg (stödord) |
|---|---|---|---|
| 0:00–0:45 | Desmond | Titelbild / README | Vi är utvecklingsteamet för MicroHydros. Ert behov: följa klimatet i ett litet odlingsskåp, få ut data och kunna varna i tid. Vår prototyp gör det med simulerade sensorer, eftersom vi inte hade hårdvara. |
| 0:45–1:45 | Desmond | Arkitekturdiagram | Sensorer, HAL, ESP32, MQTT, mottagare, databas, analys. HAL betyder att vi byter simulering mot riktiga sensorer utan att ändra resten. |
| 1:45–2:45 | Gayathri | Tabell i sensordokumentet | Varför inte DHT. SHT40 för luft och fukt inne, DS18B20 för vatten och luft ute. Viktigaste kriterierna: fuktig miljö och noggrannhet. Förslag, inte testat med hårdvara än. |
| 2:45–4:15 | Khaled | Wokwi + mottagarens terminal | Mätningar kommer var 5:e sekund. **Tryck på blå knappen:** sensorfel syns, de andra mätpunkterna fortsätter, inget falskt larm. **Tryck på orange knappen:** temperaturen stiger, varningen kommer före larmet och den röda lysdioden tänds. |
| 4:15–5:15 | Khaled | Testrapportens tabell | Vad vi testade, förväntat mot faktiskt resultat, vad som återstår. |
| 5:15–6:00 | Desmond | Graf från analysen | Historik: dygnscykel, felen syns som saknade värden och inte som 0, värmehändelsen och larmgränserna. |
| 6:00–7:00 | Desmond | Handover-dokumentet | Begränsningar: simulerat, publik broker, buffert endast i RAM. Nästa steg: köp sensorerna, bekräfta gränsvärdena, egen broker med TLS. Tack. |

## Kom ihåg
- Säg tydligt att värdena är **simulerade**.
- Larmgränserna är antaganden som kunden ska bekräfta.
- Ingen kod på skärmen.
