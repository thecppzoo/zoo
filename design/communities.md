Communities, from the perspective of suits:

### Note: whenever mentioning specific suits, the reader must multiply the different cases in which different suits can be selected for the same roles.  For example, four spades and one hears means is an abbreviation for all the ways in which a suit can be selected for four cards, and another for one card.

1. All of the same suit: `Ns*(Nr choose 5)`: 4*(13 choose 5)
2. Four spades, one heart: `Ns*(Ns - 1)*(Nr choose 4)*Nr`: 4*3*(13 choose 4)*13
3. Three spades, two hearts: `Ns*(Ns - 1)*(Nr choose 3)*(Nr choose 2)`: 4*3*(13 choose 3)*(13 choose 2)
4. Three spades, one heart, one diamond: `Ns*((Ns - 1) choose 2)*(Nr choose 3)*Nr*Nr`: 4*3*(13 choose 3)*13*13
5. Two spades, two hearts, one diamond: `(Ns choose 2)*(Ns - 2)(Nr choose 2)^2*Nr`: (4 choose 2)*2*(13 choose 2)^2*13
6. Two spades, one heart, one diamond, one club: `Ns*((Ns - 1) choose 3)*(Nr choose 2)*Nr^3`: 4*(13 choose 2)*13^3

0005148 : 00.2%
0111540 : 04.3%
0267696 : 10.3%
0580008 : 22.3%
0949104 : 36.5%
0685464 : 26.4%
======
0000030
000023
00017
0027
027
23
======
2598960
