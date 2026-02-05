1. Konzept: "Neo-Clinical Industrialism"

Vergiss "User Delight". Wir zielen auf "User Competence".

    Für die Ärzte: Es wirkt vertraut, weil es die Ästhetik von gedruckten Laborberichten und teuren Medizingeräten zitiert. Hoher Kontrast, keine versteckten Menüs.

    Für den Freshness-Faktor: Wir nutzen Mikro-Interaktionen, Monospace-Fonts und ein striktes Grid-System, das sich anfühlt wie das HUD eines Kampfjets oder eine Terminal-Schnittstelle aus der Zukunft.

2. Die Farbpalette: "Sterile & Slate"

Kein reines Schwarz (#000), kein reines Weiß (#FFF). Das blendet oder wirkt billig. Wir nutzen "Off-White" und "Deep Industrial Grey".

    Basis (Light Mode - Standard für Ärzte):

        Background: #F4F5F7 (Ganz leichtes Grau, nimmt den Schmerz aus den Augen).

        Surface (Cards/Tables): #FFFFFF (Reinweiß für Fokusbereiche).

        Text Primary: #1A1C20 (Fast Schwarz, hoher Kontrast).

        Text Secondary: #5E6C84 (Technisches Grau für Labels).

    Akzente (max 10%):

        Bio-Hazard Green: #CCFF00 oder #10B981 (Für "Success" oder "Validiert"). Extrem sparsam nutzen.

        Critical Alert: #FF3B30 (Nur für echte Fehler).

        System Blue: #0055FF (Für primäre Aktionen/Buttons). Wirkt seriös, aber elektrisch.

    Dark Mode:

        Background: #0D0E12 (Tiefes Anthrazit).

        Surface: #16181D.

        Text: #E0E0E0.

3. Typografie: "Data is King"

Wir trennen Interface und Daten strikt. Das schafft Ordnung im Chaos.

    UI-Font (Navigation, Headlines): Inter oder Helvetica Now. Sachlich, Schweizer Stil. Kein Schnörkel.

        Fett und Groß für Überschriften (z.B. "Sample ID").

    Data-Font (Werte, IDs, Ergebnisse): JetBrains Mono oder Roboto Mono.

        Warum? Tabellarische Ziffern (Zahlen stehen genau untereinander). Das lieben Ärzte, weil es Fehler beim Überfliegen verhindert. Es sieht aus wie Code, ist aber extrem lesbar.

4. Layout & Komponenten-Architektur

Wir nutzen das "Bento Box" Grid. Alles ist in klar definierten Containern. Keine schwebenden Elemente.
A. Die Navigation (Sidebar)

    Aktuell: Links klebt eine blaue Textwüste.

    Neu: Eine schmale, dunkelgraue Leiste ("Anthrazit"). Nur Icons (medizinisch/technisch) + Tooltips.

    Erweitert sich beim Hover. Das spart Platz für das Wesentliche: Die Daten.

B. Tabellen (Das Herzstück)

    Zeilenhöhe: Reduziere White-Space ("Cozy", nicht "Spacious"). Ärzte wollen Dichte.

    Zebra-Striping: Ja, aber extrem subtil (Grau auf Weiß). Führt das Auge.

    Status-Badges: Keine runden, weichen Buttons. Rechteckige Tags mit 2px Border-Radius.

        Z.B. [PENDING] in Gelb mit schwarzer Schrift (Baustellen-Optik).

        Z.B. [VALIDATED] in Grün-Outline.

    Action Buttons: Rechtsbündig. "Edit" und "Delete" sind keine Textlinks mehr, sondern Icons mit klarem Hover-State (Mechanisches Feedback).

C. Eingabemasken (Create Sample)

    Aktuell: Eine vertikale Liste des Grauens.

    Neu: Grouped Fields.

        Box 1: "Patient Context" (ID, Name).

        Box 2: "Sample Meta" (Typ, Datum).

        Nutze den gesamten Bildschirm. Ein Arzt hat einen 24-Zoll Monitor, kein Handy. Nutze das Grid (2- oder 3-Spaltig).

        Input-Felder haben keine weichen Schatten. Harte 1px Borders in Grau (#E2E8F0). Bei Fokus wird der Border #0055FF und dicker.

5. Der "Trick" für die Ärzte (Die Brücke)

Du verkaufst ihnen das Design nicht als "modern", sondern als "Fehlerreduzierend".

    Hoher Kontrast = "Sicherheit".

    Monospace Font = "Präzision".

    Keine Animationen, die länger als 0.2s dauern = "Geschwindigkeit".

Das System darf sich nicht "weich" anfühlen. Es muss "klicken". Jeder Klick ist eine definitive Entscheidung.
Visuelle Referenz (Mental Image)

Stell dir vor, Braun (Design) und Palantir (Tech) hätten ein Kind, das in einem Berliner Bunker Techno hört, aber tagsüber als Chirurg arbeitet.

    Ecken: Leicht abgerundet (4px). Nicht 0px (zu aggressiv), nicht 12px (zu verspielt).

    Schatten: Fast keine. Wir arbeiten mit Borders und Layern. "Flat", aber mit Hierarchie durch Graustufen.
