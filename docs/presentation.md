# The Payment Jacket: how we built it

Slide-ready summary for a non-technical audience. Each slide has a title, a few bullets for the slide,
and a short note on what to say.

---

## Slide 1: The idea

- A jacket covered in LED lights
- Every time someone sends us a payment, the jacket lights up
- Bigger payments get a bigger light show

*Say:* We wanted to turn an invisible event, money arriving on a phone, into something everyone in
the room can see.

---

## Slide 2: What we started with

- A store-bought LED jacket with a small battery controller
- It normally works with the maker's phone app ("Hello Fairy")
- The app only works by hand: there's no way to connect it to anything else

*Say:* The jacket could already do colours and effects. The problem was that only a person tapping
an app could control it.

---

## Slide 3: Step 1: Learning the jacket's language

- The controller talks to the app over Bluetooth, the same as wireless headphones
- Other hobbyists had already published the "words" these controllers understand
- We tested them: we sent "lights off", then "lights on", and the jacket obeyed

*Say:* It's like finding a phrasebook for a language. Once we knew the phrases for "turn off" and
"turn red", we didn't need the official app any more.

---

## Slide 4: Step 2: Making our own light shows

- Switching colours, a smooth rainbow, a fast "disco" strobe
- We tried built-in effects and worked out what looks good on the jacket
- One limit: the whole jacket changes colour together; single lights can't be controlled one by one

*Say:* We played with timing a lot. A rainbow that's too slow looks boring, and one that's too fast
just flickers.

---

## Slide 5: Step 3: Listening for payments

- Our payment wallet can send a live message every time a payment arrives
- The message says how much was paid
- Our program listens for these messages around the clock

*Say:* Think of it as a doorbell. The wallet rings it on every payment, and our program is always
listening for that ring.

---

## Slide 6: Step 4: Connecting the two

- Payment arrives, the program hears it, and it tells the jacket over Bluetooth to light up
- It takes about a second from payment to light
- We tested it with real payments, and it worked every time

*Say:* This is the moment it came together: we sent a payment from a phone, and the jacket lit up
straight away.

---

## Slide 7: Bigger payments, bigger show

| Payment size | What the jacket does |
|---|---|
| Small (under 1,000 sats) | A soft 2-second glow in yellow |
| Medium (1,000 to 9,999 sats) | A 3-second disco rainbow |
| Large (10,000 sats or more) | A 10-second show: rainbow, then disco strobe, then white flashes |

- Between payments the jacket stays dark, so every payment really stands out

*Say:* We wanted bigger payments to feel special. The big show builds up like fireworks.

---

## Slide 8: Step 5: Taking the laptop out of the picture

- At first, a laptop had to stay nearby to run everything
- Now a tiny computer board the size of a matchbox does the job (an "ESP32", costs a few euros)
- It connects to Wi-Fi, listens for payments and talks to the jacket, all by itself

*Say:* The laptop proved the idea worked. The small board makes it something you can actually wear
and take anywhere.

---

## Slide 9: Easy to set up, no programming needed

- On first start, the board creates its own Wi-Fi network called "Jacket-Setup"
- You connect with your phone, and a settings page opens, like hotel Wi-Fi
- From there you choose the Wi-Fi, the payment wallet, colours and show lengths, and press "Test"
- It's password-protected, so only we can change the settings

*Say:* Setting up the jacket shouldn't need a programmer, so everything is done on a simple page on
your phone.

---

## Slide 10: Sharing it

- All the instructions and software are public online, so friends can build their own
- Installing the software on a new board is one command
- Updates can be installed over Wi-Fi, so there's no need to plug anything in

*Say:* Anyone with the same jacket and a few euros for the board can copy what we did.

---

## Slide 11: What we learned

- Start small: test one simple thing first (on and off) before building the whole system
- Reuse what others have discovered, and don't reinvent it
- Get it working with a laptop first, then make it portable
- Make it easy for non-technical people to set up and use

---

## Slide 12: Where it stands today

- Working: the jacket reacts live to real payments with three different shows (tested with the laptop)
- Built and ready: the small stand-alone board with its phone settings page
- Next: wear-testing the stand-alone board at a real event

*Say:* The laptop version has run with dozens of real payments. The stand-alone board is finished,
and the next step is trying it out live.

---

### One-sentence summary

We taught a store-bought LED jacket to listen for payments and put on a light show for each one,
then shrank the whole setup onto a tiny, phone-configurable board that anyone can copy.
