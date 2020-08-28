int rzeros(unsigned int mask)
{
  static const char magic[32] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };

  return magic[((mask & -mask) * 0x077CB531UL) >> 27];
}

/*
**  00000 111011111001011010100110001
** 0 00001 11011111001011010100110001
** 00 00011 1011111001011010100110001
** 000 00111 011111001011010100110001
** 0000 01110 11111001011010100110001
** 00000 11101 1111001011010100110001
** 000001 11011 111001011010100110001
** 0000011 10111 11001011010100110001
** 00000111 01111 1001011010100110001
** 000001110 11111 001011010100110001
** 0000011101 11110 01011010100110001
** 00000111011 11100 1011010100110001
** 000001110111 11001 011010100110001
** 0000011101111 10010 11010100110001
** 00000111011111 00101 1010100110001
** 000001110111110 01011 010100110001
** 0000011101111100 10110 10100110001
** 00000111011111001 01101 0100110001
** 000001110111110010 11010 100110001
** 0000011101111100101 10101 00110001
** 00000111011111001011 01010 0110001
** 000001110111110010110 10100 110001
** 0000011101111100101101 01001 10001
** 00000111011111001011010 10011 0001
** 000001110111110010110101 00110 001
** 0000011101111100101101010 01100 01
** 00000111011111001011010100 11000 1
** 000001110111110010110101001 10001
** 0000011101111100101101010011 00010
** 00000111011111001011010100110 00100
** 000001110111110010110101001100 01000
** 0000011101111100101101010011000 10000
*/
