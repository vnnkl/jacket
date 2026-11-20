# Other lights that may work

This project talks to lights that are normally controlled with the **Hello Fairy** app (published by
Hysiry Technology). We have only tested one: the
[Cyberdog light-up raincoat](https://www.cyberdog.net/products/light-up-raincoat)
(controller `RSL100`, firmware 1.1.0). Everything below is *likely* to work for on/off, colour and
the payment shows, but is untested.

**Quick check:** if the product's manual or listing says "Hello Fairy app", or your phone sees a
Bluetooth device named `Hello Fairy-XXXX`, it very probably works.

## Products found that use the Hello Fairy app

Research from November 2026. Several retailers block automated reading, so some entries come from
search-result snippets; "listing" means the product page itself names the app.

| Product | Type | Evidence |
|---|---|---|
| [Avatar Controls Bluetooth lights](https://www.avatarcontrols.com/pages/fairy-lights-app-download) (curtain, icicle, waterfall, cone tree, palm tree, snowflake, pathway) | Decorative | Brand page names Hello Fairy |
| [Avatar Controls smart curtain, 400 LEDs](https://www.avatarcontrols.com/products/smart-curtain-lights-string-with-400-rgb-leds-diy-your-effects) | Curtain | Manual names "Hello Fairy-xxx" devices |
| [Avatar Controls smart C9 string lights, 200 count (Lowe's)](https://www.lowes.com/pd/Avatar-Controls-Smart-C9-String-Lights-200-Count-132-ft-Multi-function-Multicolor-LED-Plug-In-Christmas-String-Lights-Timer/5016923097) | String lights | Listing names the app |
| [RGB Christmas net lights, 10x10 ft (Amazon)](https://us.amazon.com/Christmas-Control-Outdoor-control-outdoor/dp/B0H4LZV592) | Net lights | Listing title names the app |
| [Ivy Bronx mini RGB cluster lights, 10 m (Wayfair)](https://www.wayfair.ie/home/pdp/ivy-bronx-mini-rgb-led-cluster-lights-10m-i110744077.html) | String lights | Listing names the app |
| [C9 RGBW string lights, 2 x 33 ft (Amazon)](https://www.amazon.com/Christmas-String-Outdoor-Control-Controller/dp/B0GZDVQ54J) | String lights | Search snippet |
| [ICLOVER 400-LED curtain (Walmart)](https://www.walmart.com/ip/12150965130) | Curtain | Search snippet |
| [RGB curtain, 400 LEDs, 2 x 2 m (Newegg)](https://www.newegg.com/p/0VV-025F-00001) | Curtain | Search snippet |

Other projects have used: a 20x20 Hello Fairy curtain (model 25025,
[hello_fairy_ble](https://github.com/nzrutman/hello_fairy_ble)) and a string light (model BSL84,
[fairy-lights](https://github.com/Craterbrain/fairy-lights)). Hello Fairy's own catalogue is at
[hellofairylighting.com](https://www.hellofairylighting.com/).

We found **no other clothing or wearables** using this app; the raincoat seems to be unusual.

## What carries over, and what doesn't

- **Same everywhere so far:** the Bluetooth service, power on/off and whole-light colour commands.
  So `jacket on/off/color`, the payment shows and the ESP32 firmware should work as they are.
- **Different per model:** controlling *single LEDs* (pictures and animations). Curtain and string
  models with firmware 4.x support it; our jacket (firmware 1.x) doesn't seem to.
- **Look-alikes:** some similar-looking products use other apps (for example "Surplife"). Those use
  a different protocol and won't work.
- Firmware can be updated through a separate "Hello Fairy OTA" app, so behaviour may change.

If you get another product working, please open an issue with its name, the `Hello Fairy-XXXX`
Bluetooth name, and the model and firmware from the device-info reply.
