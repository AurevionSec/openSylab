1. Concept: "Neo-Clinical Industrialism"

Forget "User Delight". We aim for "User Competence".

    For physicians: It feels familiar, because it references the aesthetics of printed laboratory reports and expensive medical devices. High contrast, no hidden menus.

    For the freshness factor: We use micro-interactions, monospace fonts, and a strict grid system that feels like the HUD of a fighter jet or a terminal interface from the future.

2. Color Palette: "Sterile & Slate"

No pure black (#000), no pure white (#FFF). These either blind or look cheap. We use "Off-White" and "Deep Industrial Grey".

    Base (Light Mode — default for physicians):

        Background: #F4F5F7 (Very light grey, takes the pain out of your eyes).

        Surface (Cards/Tables): #FFFFFF (Pure white for focus areas).

        Text Primary: #1A1C20 (Near black, high contrast).

        Text Secondary: #5E6C84 (Technical grey for labels).

    Accents (max 10%):

        Bio-Hazard Green: #CCFF00 or #10B981 (For "Success" or "Validated"). Use extremely sparingly.

        Critical Alert: #FF3B30 (Only for genuine errors).

        System Blue: #0055FF (For primary actions/buttons). Looks serious, but electric.

    Dark Mode:

        Background: #0D0E12 (Deep anthracite).

        Surface: #16181D.

        Text: #E0E0E0.

3. Typography: "Data is King"

We strictly separate interface and data. This creates order in the chaos.

    UI Font (Navigation, Headlines): Inter or Helvetica Now. Matter-of-fact, Swiss style. No flourishes.

        Bold and large for headings (e.g. "Sample ID").

    Data Font (values, IDs, results): JetBrains Mono or Roboto Mono.

        Why? Tabular figures (numbers align precisely in columns). Physicians love this because it prevents errors when scanning. It looks like code, but is extremely readable.

4. Layout & Component Architecture

We use the "Bento Box" grid. Everything lives in clearly defined containers. No floating elements.

A. Navigation (Sidebar)

    Current: A blue wall of text stuck to the left.

    New: A narrow, dark-grey bar ("Anthracite"). Icons only (medical/technical) + tooltips.

    Expands on hover. This saves space for what matters: the data.

B. Tables (The Core)

    Row height: Reduce white space ("Cozy", not "Spacious"). Physicians want density.

    Zebra striping: Yes, but extremely subtle (grey on white). Guides the eye.

    Status badges: No rounded, soft buttons. Rectangular tags with 2px border-radius.

        E.g. [PENDING] in yellow with black text (construction-site aesthetic).

        E.g. [VALIDATED] in green outline.

    Action buttons: Right-aligned. "Edit" and "Delete" are no longer text links — they are icons with a clear hover state (mechanical feedback).

C. Input Forms (Create Sample)

    Current: A vertical list of misery.

    New: Grouped fields.

        Box 1: "Patient Context" (ID, Name).

        Box 2: "Sample Meta" (Type, Date).

        Use the full screen. A physician has a 24-inch monitor, not a phone. Use the grid (2 or 3 columns).

        Input fields have no soft shadows. Hard 1px borders in grey (#E2E8F0). On focus the border becomes #0055FF and thicker.

5. The "Trick" for Physicians (The Bridge)

You don't sell them the design as "modern" — you sell it as "error-reducing".

    High contrast = "Safety".

    Monospace font = "Precision".

    No animations lasting longer than 0.2s = "Speed".

The system must not feel "soft". It must "click". Every click is a definitive decision.

Visual Reference (Mental Image)

Imagine Braun (design) and Palantir (tech) had a child who listens to techno in a Berlin bunker, but works as a surgeon by day.

    Corners: Slightly rounded (4px). Not 0px (too aggressive), not 12px (too playful).

    Shadows: Almost none. We work with borders and layers. "Flat", but with hierarchy through grey scales.
