## Hi, and thanks for helping with uTox. We're all glad for the support!

The structure for the layout/ directory is a bit different. All of
the #includes should be placed inline instead of the header; ideally right
before the first use. But thats a loose requirement.

Apart from that, the PRIMARY goal should be readability, a close second would
be easy of editing, meaning fewer files are probably better. But #include
exists for a reason. Make the code easy to read, then both my and your life
easy and everyone will be happy!

Thanks again! https://cmdline.org/who.png
