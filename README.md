![1000000572](https://github.com/user-attachments/assets/6496a7e0-6ee0-43b7-8986-386c78e023d7)

![demo video](https://www.youtube.com/watch?v=A4CuWbXQJcw)

Display humidity & temperature through framebuffer. (w/ Weather-Board-Zero)

You need to compile the Linux kernel with framebuffer options enabled and connect OLED which is portable with ssd1306 / ssd1315.

Then, you can check framebuffer files in your `/dev` directory.

```
$ ls /dev/fb*
/dev/fb0  /dev/fb1
```

You need to connect the Weather-Board-Zero module to gather information about humidity and temperature.
(![Weather-Board-Zero](https://www.odroid.nl/WBZ)) This module is made for ODROID models in hardkernel, but I think it's possible to connect with other board which supports I2C protocol.

For me, I also used MULTIIO Training Board for ODROID-M1S, so maybe you need other gadgets to make this thing work as expected.
(![MUITIIO](https://www.hardkernel.com/ko/shop/multi-i-o-training-board-for-m1s/))

It's just a toy program, so don't expect too much about functionality!

But I think it has some interesting topics, like displaying bitmap fonts in monochrome screen etc...

By the way, sorry for dirty code XD
