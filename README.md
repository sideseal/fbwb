![1000000572](https://github.com/user-attachments/assets/6496a7e0-6ee0-43b7-8986-386c78e023d7)

Display humidity & temperature through framebuffer.
You need to compile the Linux kernel with framebuffer options enabled and connect OLED which is portable with ssd1306 / ssd1315.
Then, you can check framebuffer files in your `/dev` directory.

```
$ ls /dev/fb*
/dev/fb0  /dev/fb1
```

Also, you need to connect the Weather-Board-Zero module to gather information about humidity and temperature.
![Weather-Board-Zero](https://www.odroid.nl/WBZ)

For me, I also used MULTIIO Training Board for odroid-m1, so maybe you need other gadgets to make this thing work as expected.
![MUITIIO](https://www.hardkernel.com/ko/shop/multi-i-o-training-board-for-m1s/)

This program runs as a daemon, so if you need to stop it, enter `pkill fbwb` in your command line.
If you want to check the status of the daemon, you can see a `syslog` which is located in `/var/log/` directory.

It's just a toy program, so don't expect too much about functionality!
But I think it has some interesting topics, like scrolling up the screen, displaying bitmap fonts in monochrome etc...
By the way, sorry for dirty code XD
