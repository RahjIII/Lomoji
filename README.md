# LibLomoji - The Last Outpost Emoji Translation Library

## Description

Liblomoji is a UTF-8 ⇔ ASCII translation library written in C.  It uses files
written in the XML format provided by UTS #35: Unicode Locale Data Markup
Language (LDML) to map Unicode character sequences (graphemes) to useful,
locale specific names.  It also has a built in set of "ASCII equivalent
characters" for mapping single Unicode codepoints into ASCII letters.

What does that mean exactly?  

Liblomoji lets you easily translate ASCII strings with markup into UTF-8
emojis, and emojis back into ASCII markup, like this:

`:whoa: ➡ 😵‍💫 ➡ :face_with_spiral_eyes:`

The annotation files from The Unicode Common Locale Data Repository (CLDR) are
a great source of localized "text to speech" mappings for emoji, and liblomoji
was designed to make it easy layer on your own emoji to name mappings as well.
The liblomoji library was written specifically for use in Multi-User Dungeon
text adventure games to provide useful, readable translation of emoji
characters into ASCII, and for entering emoji by name.

On a MUD such as [The Last Outpost MUD](https://www.last-outpost.com),
some people play from a phone client and have easy access to emoji characters,
but others may be playing from an older client, or even from a physical
terminal that is ASCII only and can't display emoji directly.  Others may be
playing from a PC with a physical keyboard and able to see emoji, but have a
tough time entering emoji directly.  Liblomoji provides a translation layer for
those players.

While it was written specifically for easy integration with existing C MUD
code, Liblomoji can be use anywhere that emoji translation may be useful.


## Compilation, Installation

Liblomoji requires glib-2.0, and strongly recommends use of the annotation
files from the CLDR project.  See the INSTALL file for more instruction.

By default, liblomoji tries to merge annotations from three filenames in order:
`default.xml` `derived.xml` `extra.xml`.  It looks for that set of filenames
first in `/usr/local/share/lomoji` and second in `$HOME/.lomoji`.  Definitions
loaded last take priority.

The install script creates symlinks for `default.xml` and `derived.xml` that
point them to the English versions of `annotations/en.xml` and
`annotationsDerived/en.xml` respectively.  To to set up a different default
language system wide, change the symlinks to point to a different CLDR language
set.  

The `extra.xml` file is loaded last so that it can be used to override any CLDR
annotations with local custom ones.  The installer symlinks `extra.xml` to the
`example_extra.xml` file by default, but the example contains some specific
overrides for The Last Outpost MUD that you might not want.  The annotations
files do not have to exist, and it is only an error if no annotations can be
loaded whatsoever, so you might want to break the extra.xml link, or point it
to your own overrides.

Note that annotations with the `type="tts"` attribute are the ones used for
Unicode to ASCII translation. Other annotations are used as aliases, and are
used during ASCII to Unicode translation along with the ones marked tts.


## Usage

Read the lomoji.h file (installed in /usr/share/include by default) for a more
complete description of the programming API.  

This is overview- For basic use, call `lomoji_init()` at the start of the
program, call `lomoji_from_ascii()` and `lomoji_to_ascii()` as needed (they
work like `strdup()`), and call `lomoji_done()` before exiting.  A simple
program might look like this:

```c
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
```

To build this example, the compiler needs some extra cflags and libs flags in
order to find the library and header files, as it would with any other library.
The easiest way is with the pkg-config command.

```
gcc `pkg-config --cflags lomoji` example.c -o example `pkg-config --libs lomoji`
```

In a makefile, you might add the pkg-config output to your flags and libs
variables like this:
```
CFLAGS += `pkg-config --cflags lomoji`
LINKLIBS += `pkg-config --libs lomoji`
```

## What I've learned from this project:

Unicode and Emoji handling is *difficult*.  There are basic localized Unicode
to ASCII translators available for or built into Linux (like iconv) but
frankly, they leave a lot to be desired.  They are either too hard to use, or
too... I don't know, *Unicodey*, to be useful in a game.  Yes, I know that
Unicode U+200d is named "ZERO WIDTH JOINER", but how is it useful for people
playing a game to read `:vampire::ZERO WIDTH JOINER::female_sign:`?  

If I could have found a library that worked half as well as liblomoji does, I
wouldn't have written all this code.  But I did, so here we are.


## Bugs, Limitations, Todos

Liblomoji makes no claims to be bug-free, complete, perfect, or any other such
nonsense.  If you find it useful, great!  If you find a bug, let me know.


## Contributing, Bug Reporting, Support

Contributions, suggestions, bug reports are all welcome.  Please contact the
primary author in the AUTHORS file for more info.


## Thanks

Please see the AUTHORS file for a full list of contributors!

