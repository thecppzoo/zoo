The Floyd sampling algorithm --you can see an excellent exposition (here)[http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html]-- is very convenient for use cases such as getting a hand of cards from a deck.

The fastest way to represent sets of finite and small domains (such as a deck of cards) seems to be as bits in bitfields.

For an example of a deck of 52 cards, we may want, for example, to generate all of the 7-card hands.  I wrote an straightforward implementation (here)[https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/Floyd.h].
