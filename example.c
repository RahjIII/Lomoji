/* Simple C program to display "Hello World !!" */
#include <stdio.h>
#include <lomoji.h>

int main()
{
    char hello[] = ":hello: :world: :bangbang:\n";
    char *utf;
    char *ascii;

    /* init the library */
    lomoji_init();

    /* Translate into UTF-8 */
    utf = lomoji_from_ascii(hello);
    printf("%s", utf);

    /* Translate back to ASCII */
    ascii = lomoji_to_ascii(utf);
    printf("%s", ascii);

    /* free the strings when done. */
    free(ascii);
    free(utf);

    /* clean up the library */
    lomoji_done();

    return (0);
}
