# Remote-FX-AudioLight
An RFM69 feather based module to add audio and light effects to table role playing games.

The TX code is premade with 4 pages already populated with commands to send (there is no encryption so you can use it with Circuit Python boards as well ) :
Page 1 from 'A' to 'P' (uppercase)
Page 2 from 'a' to 'p' (lowercase)
Page 3 from 'Q' to 'Z' for the first 2 rows and from 'q' to 'z' for the last 2 rows
Page 4 is pure ASCII character from '1' to '16'

The RX code is basically John Park's RX NeoPixel code with an addition of Music Maker example code and some i added.
On lines 164 and 292, you'll find a way to choose an audio file randomly based on it's name (the base name is on line 34 : char FILENAME[] = "ghost01.wav";  the idea is i have 6 files from ghost01 to ghost 06 and it randomly choose a number to launch the file by replacing the 7th character of the name).

<iframe width="560" height="315" src="https://www.youtube.com/embed/ErXqcd3CuBw" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
