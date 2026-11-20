---
marp: true
size: 16:9
paginate: true
title: The payment jacket
description: How we taught an LED jacket to light up for every payment
---

<style>
@font-face { font-family: 'Bricolage Grotesque'; font-weight: 200 800; font-display: block; src: url('fonts/BricolageGrotesque.woff2') format('woff2'); }
@font-face { font-family: 'Lexend'; font-weight: 100 900; font-display: block; src: url('fonts/Lexend.woff2') format('woff2'); }

:root {
  --denim: #1B2340;
  --denim-deep: #141A31;
  --chalk: #EEF0F7;
  --dusk: #9AA3C2;
  --yellow: #FFC93C;
  --pink: #FF5FA2;
  --mint: #3DD6B0;
  --violet: #9B8CFF;
  --dark-bulb: #343D5E;
  /* bulbs on the strand, left to right; slides override these */
  --b1: var(--yellow); --b2: var(--pink); --b3: var(--mint); --b4: var(--violet);
}

section {
  background: radial-gradient(120% 90% at 85% 110%, #232D52 0%, var(--denim) 55%, var(--denim-deep) 100%);
  color: var(--chalk);
  font-family: 'Lexend', system-ui, sans-serif;
  font-weight: 400;
  font-size: 33px;
  line-height: 1.45;
  padding: 104px 104px 72px;
  justify-content: center;
}

/* The fairy-light strand: a wire with four repeating bulbs, each with a soft glow. */
section::before {
  content: '';
  display: block !important; /* Marp's default theme makes this a zero-width table */
  position: absolute;
  top: 26px; left: 0; right: 0; width: 100%;
  height: 44px;
  background:
    radial-gradient(circle at 20px 30px, var(--b1) 0 7px, color-mix(in srgb, var(--b1) 35%, transparent) 9px, transparent 20px) 0 0 / 160px 44px repeat-x,
    radial-gradient(circle at 60px 30px, var(--b2) 0 7px, color-mix(in srgb, var(--b2) 35%, transparent) 9px, transparent 20px) 0 0 / 160px 44px repeat-x,
    radial-gradient(circle at 100px 30px, var(--b3) 0 7px, color-mix(in srgb, var(--b3) 35%, transparent) 9px, transparent 20px) 0 0 / 160px 44px repeat-x,
    radial-gradient(circle at 140px 30px, var(--b4) 0 7px, color-mix(in srgb, var(--b4) 35%, transparent) 9px, transparent 20px) 0 0 / 160px 44px repeat-x,
    linear-gradient(transparent 21px, #4A547A 21px 22px, transparent 22px) 0 0 / 100% 44px no-repeat;
}

/* Strand moods, used per slide with <!-- _class: ... --> */
section.dark   { --b1: var(--dark-bulb); --b2: var(--dark-bulb); --b3: var(--dark-bulb); --b4: var(--dark-bulb); }
section.one    { --b1: var(--yellow); --b2: var(--dark-bulb); --b3: var(--dark-bulb); --b4: var(--dark-bulb); }
section.yellow { --b1: var(--yellow); --b2: var(--yellow); --b3: var(--yellow); --b4: var(--yellow); }
section.white  { --b1: #FFFFFF; --b2: var(--yellow); --b3: #FFFFFF; --b4: var(--pink); }

h1, h2 {
  font-family: 'Bricolage Grotesque', system-ui, sans-serif;
  font-weight: 800;
  letter-spacing: -0.02em;
  line-height: 1.05;
  margin: 0 0 28px;
  color: var(--chalk);
}
h1 { font-size: 96px; max-width: 14ch; }
h2 { font-size: 60px; max-width: 20ch; }

/* "Step 2 of 5" — the build really was a sequence */
.step {
  font-family: 'Bricolage Grotesque', system-ui, sans-serif;
  font-weight: 500;
  font-size: 28px;
  color: var(--yellow);
  margin: 0 0 10px;
}

p, li { max-width: 34em; }
ul { padding-left: 1.1em; margin: 0; }
li { margin: 0 0 14px; }
li::marker { color: var(--dusk); }
strong { color: var(--yellow); font-weight: 600; }
em { color: var(--dusk); font-style: normal; }

.lede { font-size: 36px; line-height: 1.35; max-width: 24em; color: var(--chalk); }
.quiet { color: var(--dusk); font-size: 26px; margin-top: 28px; max-width: 46em; }

/* The flow on slide 6: payment -> listener -> jacket */
.flow { display: flex; align-items: center; gap: 22px; margin: 40px 0 36px; font-size: 30px; }
.flow span { padding: 18px 26px; border: 2px solid #4A547A; border-radius: 999px; white-space: nowrap; }
.flow b { color: var(--dusk); font-weight: 400; font-size: 34px; }
.flow span:last-child { border-color: var(--yellow); color: var(--yellow); }

/* The three shows: each row starts with a little bulb strip in that show's colours */
section table { border-collapse: collapse; font-size: 31px; margin-top: 8px; display: table; width: 100%; }
section table td:nth-child(2) { white-space: nowrap; }
section table th, section table td { padding: 20px 32px 20px 0; border: 0 !important; border-bottom: 1px solid #39426A !important; background: transparent !important; vertical-align: middle; }
section table tr:last-child td { border-bottom: 0 !important; }
section table thead { display: none; }
tr { background: transparent !important; }
.bulbs { display: inline-flex; gap: 10px; }
.bulbs i { width: 18px; height: 18px; border-radius: 50%; display: inline-block; box-shadow: 0 0 14px currentColor; background: currentColor; }

/* Two columns: text left, picture right */
.split { display: grid; grid-template-columns: 1.15fr 0.85fr; gap: 56px; align-items: start; }
.split img { width: 100%; border-radius: 22px; box-shadow: 0 30px 60px rgba(5, 8, 20, 0.55); }
.phone { height: 500px; overflow: hidden; border-radius: 26px; border: 2px solid #4A547A; }

section::after { color: var(--dusk); font-size: 20px; } /* page number */
section.title::after { display: none; }
section.title { padding-top: 96px; }
section.title h1 { font-size: 120px; }
</style>

<!-- _class: title dark -->
<!-- _paginate: false -->

# The payment jacket

<p class="lede">A store-bought LED jacket that puts on a light show every time someone pays us.</p>

<!--
We wanted to turn something invisible, money arriving on a phone, into something everyone in the room can see.
-->

---

<!-- _class: dark -->

## Where we started

- An LED jacket with a small battery controller
- It only works with the maker's phone app
- Someone had to tap the app: nothing else could control it

<!--
The jacket could already do colours and effects. The problem: only a person with the app could make it do anything.
-->

---

<!-- _class: one -->

<p class="step">Step 1 of 5</p>

## Learning the jacket's language

- The controller listens over Bluetooth, like wireless headphones
- Hobbyists had already written down the commands it understands
- Our first test: "lights off", then "lights on", and it worked

<!--
It's like finding a phrasebook. Once we knew the phrases for "turn off" and "turn red", we didn't need the official app any more.
-->

---

<p class="step">Step 2 of 5</p>

## Making our own light shows

- Single colours, smooth rainbows, a fast disco strobe
- Lots of tweaking: too slow is boring, too fast just flickers
- One limit: the whole jacket shows one colour at a time

<!--
We tried every built-in effect and our own, and tuned the timing until it looked good on the jacket.
-->

---

<!-- _class: one -->

<p class="step">Step 3 of 5</p>

## Listening for payments

- Our wallet sends a live message for every payment
- The message says how much was paid
- Our program listens around the clock, like waiting for a doorbell

<!--
The wallet rings the bell on every payment, and our program is always listening for it.
-->

---

<p class="step">Step 4 of 5</p>

## Connecting the two

<div class="flow"><span>Payment arrives</span><b>→</b><span>Program hears it</span><b>→</b><span>Jacket lights up</span></div>

- About one second from payment to light
- Tested with dozens of real payments, and the jacket responded every time

<!--
This was the moment it all came together: a payment from a phone, and the jacket lit up straight away.
-->

---

## Bigger payments, bigger show

| | | |
|---|---|---|
| <span class="bulbs" style="color:#FFC93C"><i></i><i></i><i></i></span> | **Under 1,000 sats** | A soft yellow glow, 2 seconds |
| <span class="bulbs"><i style="color:#FFC93C"></i><i style="color:#FF5FA2"></i><i style="color:#3DD6B0"></i></span> | **1,000 – 9,999 sats** | A disco rainbow, 3 seconds |
| <span class="bulbs"><i style="color:#FFFFFF"></i><i style="color:#9B8CFF"></i><i style="color:#FFFFFF"></i></span> | **10,000 sats and up** | The full light show, 10 seconds |

<p class="quiet">The jacket stays dark in between. All of this can be changed from a phone.</p>

<!--
Bigger payments should feel special. The big show builds up like fireworks: rainbow, then strobe, then white flashes. Right now we've set it to a disco for every payment, chosen on the phone.
-->

---

<!-- _class: one -->

<p class="step">Step 5 of 5</p>

## Leaving the laptop at home

- At first a laptop had to be nearby to run everything
- Now a matchbox-sized board does it all, for a few euros
- It joins Wi-Fi, listens for payments and drives the jacket by itself

<!--
The laptop proved the idea. The small board makes it something you can wear anywhere.
-->

---

<div class="split">
<div>

## Set up from your phone

- The board opens its own Wi-Fi: "Jacket-Setup"
- Join it and a settings page appears, like hotel Wi-Fi
- Pick the Wi-Fi, wallet, colours and show lengths, then press "Test"
- Password-protected, so only we can change it

</div>
<div class="phone"><img src="admin-page.png" alt="The jacket's settings page on a phone: status lights, test buttons and show settings"></div>
</div>

<!--
No programmer needed. Everything happens on a simple page on your phone.
-->

---

## Anyone can build one

- The software and instructions are public online
- Putting the software on a new board takes one command
- Updates install over Wi-Fi: no cables needed

<!--
Anyone with the same jacket and a few euros for the board can copy what we did.
-->

---

<!-- _class: yellow -->

## What we learned

- Test the smallest thing first: just on and off
- Reuse what others have already discovered
- Make it work on a laptop, then make it portable
- Make setup easy enough for anyone

---

<!-- _class: white -->

## Where it stands

- **Working:** the jacket reacts live to real payments
- **Running:** the stand-alone board, set up entirely from a phone
- **Next:** wearing it at a real event

<!--
The laptop version handled dozens of real payments, and the stand-alone board is now running on our Wi-Fi. Next we wear it live.
-->

---

<!-- _class: title -->
<!-- _paginate: false -->

# Pay us. Watch us glow.

<p class="lede">An LED jacket that listens for payments and puts on a light show for each one.</p>
