/* lomoji.c - Last Outpost Emoji Translator Library */
/* Created: Wed May 29 10:35:37 AM EDT 2024 malakai */
/* Copyright © 2024 Jeffrika Heavy Industries */
/* $Id: lomoji.c,v 1.6 2024/06/22 21:40:05 malakai Exp $ */

/* Copyright © 2024 Jeff Jahr <malakai@jeffrika.com>
 *
 * This file is part of liblomoji - Last Outpost Emoji Translation Library
 *
 * liblomoji is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * liblomoji is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with liblomoji.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>

#include "lomoji.h"

/* local #defines */
#define DEFAULT_LOCALE "en_US"  /* default for g_str_to_ascii() use. */
#define DEFAULT_TTS_PREFIX ":"
#define DEFAULT_TTS_SUFFIX ":"
#define DEFAULT_UNKNOWN "?"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#ifndef SHARE_PREFIX
#define SHARE_PREFIX "/usr/local/share"
#endif

/* structs and typedefs */

/* lomoji_ctx_t is the context structure that holds the options and translation
 * data. */
struct lomoji_ctx_s {
	char *tts_prefix;		/* marker for start of ascii name */
	char *tts_suffix;		/* marker for end of ascii name */
	char *unknown;			/* 'unknown codepoint' substitution */
	GHashTable *cp_tts;		/*codepoint to tts string.*/
	GHashTable *cp_equiv;	/*codepoint to single ascii char.*/
	GTree *alias_cp;		/*alias to codepoint. */
};

/* annotations accumulator structure, used by the XML parser. */
struct ann_acc {
	gchar *cp;
	gchar *text;
	int tts;
	lomoji_ctx_t *ctx;
};

/* a struct associating an ascii character with a utf string. */
typedef struct {
	char ascii;
	char *utf;
} lomoji_equiv_t;

/*---- local function declarations ----*/

/* callback functions for the annotations xml parser. */
static void ann_parser_start_element(GMarkupParseContext * context, const gchar * element_name, const gchar ** attribute_names,
    const gchar ** attribute_values, gpointer user_data, GError ** error);
static void ann_parser_end_element(GMarkupParseContext * context, const gchar * element_name, gpointer user_data, GError ** error);
static void ann_parser_characters(GMarkupParseContext * context, const gchar * text, gsize text_len, gpointer user_data,
    GError ** error);
static void ann_parser_passthrough(GMarkupParseContext * context, const gchar * passthrough_text, gsize text_len,
    gpointer user_data, GError ** error);
static void ann_parser_error(GMarkupParseContext * context, GError * error, gpointer user_data);
struct ann_acc *ann_acc_new(lomoji_ctx_t *ctx);
static void ann_acc_clear(struct ann_acc *acc);
static void ann_acc_free(struct ann_acc *acc);
static gint treecompare(gconstpointer a, gconstpointer b, gpointer user_data);

/* Some internal initialization and utility functions. */
int lomoji_add_equiv(lomoji_ctx_t *ctx, lomoji_equiv_t *source);
int lomoji_add_regional(lomoji_ctx_t *ctx);
void lomoji_add_oneoffs(lomoji_ctx_t *ctx);
gchar *keypart_dup(lomoji_ctx_t *ctx, char *in);

/*---- exported local variable declarations ----*/

char **lomoji_default_filepaths = NULL;

/* an example of what an annotations filepath might look like.
char *lomoji_filepath_example[] = {
	"/share/lomoji/lomoji_default_filepaths.xml",
	"lomoji_annotations.xml",
	"lomoji_local_annotations.xml",
	NULL
};
*/

/* a shared context for the 'basic' functions to use. */
lomoji_ctx_t *lomoji_default_ctx = NULL;

/* exported filter lists. */
lomoji_filter *lomoji_none[] = {
	NULL
};

lomoji_filter *lomoji_nameuplus[] = {
	filter_toname,
	filter_uplus,
	NULL
};

lomoji_filter *lomoji_toascii[] = {
	filter_toname,
	filter_equiv,
	filter_decompose,
	filter_unknown,
	NULL
};

lomoji_filter *lomoji_fromascii[] = {
	filter_fromname,
	NULL
};

lomoji_filter *lomoji_iconv[] = {
	filter_iconv,
	NULL
};

lomoji_filter *lomoji_namesonly[] = {
	filter_toname,
	NULL
};

lomoji_filter *lomoji_uplusonly[] = {
	filter_uplus,
	NULL
};

lomoji_filter *lomoji_nameiconv[] = {
	filter_toname,
	filter_iconv,
	NULL
};


/*---- local variable declarations ----*/

/* some of these translations came indirectly from http://obfuscator.uo1.net/ -jsj */
lomoji_equiv_t default_equiv[] = {
	{'^',"◬↑"},
	{'=',"⛁▰ະ▅▆⏎🙙🙛🙜🙞"},
	{'|',"│⚕┤▏▕"},
	{'_',"…▁▂"},
	{'-',"─┉▃▄🙙🙛"},
	{'!',"¡"},
	{'?',"¿"},
	{'/',"╭╯☍☄🙑🙒🙕🙖"},
	{'.',"·᛬"},
	{'(',"⚸"},
	{')',"▎"},
	{']',"▋▊"},
	{'}',"▍▌"},
	{'@',""},
	{'$',"¢€¥£"},
	{'*',"☆☼★⚙❃"},
	{'\\',"╮╰🙐🙓🙔🙗"},
	{'&',""},
	{'#',"⊞⍓▇█▉✉"},
	{' ',"᛫        "},
	{'+',"✔✓"},
	{'~',"🙘🙚"},

	{'0',"◒☯◌"},

	{'>',"⇨→⇒⇢↗»🙟"},
	{'<',"⇦←⇐⇠↘↯«🙝"},

	{'A',"AÀÁÂÃÄÅĀĂĄǍǞǠȀȂȦΆΑАѦӐӒḀẠẢẤẦẨẬẶἈἉᾈᾉᾸᾹᾺᾼ₳ÅȺẮẰẲẴἌἎἏᾌΆǺẪ♤⚜дᚨᚪᛅᛆ"},
	{'B',"BƁΒВḂḄḆвᛒᛓ"},
	{'C',"CÇĆĈĊČƇʗСҪḈ₢₵ℂⅭϹϾҀᛍ"},
	{'D',"DÐĎĐƉƊḊḌḎḐḒⅮᛑᛞ"},
	{'E',"EÈÉÊËĒĔĖĘĚȄȆȨΕЀЁЕӖḘḚḜẸẺẼẾỀỆḔḖỂỄԐℇƐἙῈЄᛂᛖ"},
	{'F',"FϜḞ₣ҒƑϝғᚠ"},
	{'G',"GĜĞĠĢƓǤǦǴḠ₲ᚵᚷ"},
	{'H',"HĤĦȞΗНҢҤӇӉḢḤḦḨḪῌꜦ♔ᚺᚻᚼᚽ"},
	{'I',"IΊÌÍÎÏĨĪĬĮİƖƗǏȈȊΙΪІЇӀӏḬḮỈỊἸἹῘῙῚǐ1ᛁ"},
	{'J',"JĴʆЈʃᛃ"},
	{'K',"KĶƘǨΚЌКԞḰḲḴ₭Kᚴ"},
	{'L',"LĹĻĽĿŁԼḶḸḺḼℒⅬ˪╚£ᛚᛛ"},
	{'M',"MΜМӍḾṀṂⅯᛗᛘᛙ"},
	{'N',"NÑŃŅŇǸΝṄṆṈṊ₦Ɲ∏⋂иᚾᚿᛀ"},
	{'O',"O0θϑ⍬ÒÓÔÕÖØŌŎŐƆƟƠǑǪǬǾȌȎȪȬȮȰΘΟϴОѲӦӨӪՕỌỎỐỒỔỘỚỜỞỠỢΌΌṌṐṒὈʘṎỖᚩᚭᚮᛟ"},
	{'P',"PƤΡРҎṔṖῬ₱ℙ✍✎ᛈᛔᛕ"},
	{'Q',"QԚℚ⍜ᛩ"},
	{'R',"RŔŖŘȐȒṘṚṜṞ℞ɌⱤЯᚱ"},
	{'S',"SŚŜŞŠȘЅՏṠṢṨṤṦ∫∬ᛊᛋᛌ"},
	{'T',"TŢŤŦƮȚΤТҬṪṬṮṰ₮ȾΊꚌ☥ᛏᛐ"},
	{'U',"UÙÚÛÜŨŪŬŮŰŲƯǓǕǗǛȔȖԱՍṲṴṶṸỤỦỨỪỬỮỰǙ⊍⊎Մ⊌Ṻ☋ᚢ"},
	{'V',"VѴѶṼṾ⋁ⅤƲᚡ"},
	{'W',"WŴԜẀẂẄẆẈ₩ƜШᚥᚹ"},
	{'X',"XΧХҲẊẌⅩ⚒ᛪ"},
	{'Y',"Y¥ÝŶŸƳȲΥΫϓУҮҰẎỲỴỶỸῨῩᚤ"},
	{'Z',"ZŹŻŽƵȤΖẐẒẔᛎ"},
	{'a',"aàáâãäåāăąǎǟǡǻȁȃȧаӑӓḁẚạảấầẩẫậắằẳẵặɑάαἀἁἂἃἄἅἆἇὰάᾀᾁᾂᾃᾄᾅᾆᾇᾰᾱᾲᾳᾴᾶᾷ⍶⍺æᚫ"},
	{'b',"bƀƃƅɒɓḃḅḇþϸƄьҍ"},
	{'c',"cçćĉċčƈςϛсҫḉⅽ¢ϲҁᛢᛣᛤ"},
	{'d',"dďđɖɗḋḍḏḑḓⅾƌժ₫ð"},
	{'e',"eèéêëēĕėęěȅȇȩеѐёҽҿӗḕḗḙḛḝẹẻẽếềểễệεɛϵєϱѳөӫɵᚯᛇᛠ"},
	{'f',"fſḟẛƒ"},
	{'g',"gĝğġģǥǧǵɠɡցḡɕʛɢᚶᚸᛄ"},
	{'h',"hĥħȟɦɧћիհḣḥḧḩḫẖℏһʜӊ☝н"},
	{'i',"iįìíîïĩīĭıȉȋɨɩΐίιϊіїɪḭḯỉịἰἱἲἳὶίῑΐῐῒῖ⚵"},
	{'j',"jĵǰȷɟʝјյϳᛡ"},
	{'k',"kķĸƙǩκкҝҟḱḳḵᚲᚳ"},
	{'l',"lŀĺļľłƚǀɫɬɭḷḹḻḽ⎩"},
	{'m',"mɱḿṁṃ₥ⅿм"},
	{'n',"nɴñńņňŉŋƞǹɲɳήηπпբդըղոռրṅṇṉṋἠἡἢἣἤἥἦἧὴήᾐᾑᾒᾓᾔᾕᾖᾗῂῃῄῆῇი☊∩лᚰ"},
	{'o',"oòóôõöōŏőơǒǫǭȍȏȫȭȯȱοόоӧծձօṍṏṑṓọỏốồổỗộớờởỡợὀὁὂὃὄὅὸόσ๐ø"},
	{'p',"pρрҏթṕṗῤῥ⍴"},
	{'q',"qʠԛգզϙ"},
	{'r',"rŕŗřȑȓɼɽгѓґӷṙṛṝṟ"},
	{'s',"sśŝşšșʂѕԑṡṣṥṧṩßᛥ"},
	{'t',"tţťŧƫțʈṫṭṯṱẗȶէե†ԷՒȽҭ☨тᚦᚧ"},
	{'u',"uµùúûüũūŭůűųưǔǖǘǚǜȕȗɥμυцկմնսվևṳṵṷṹṻụủứừửữự"},
	{'v',"vʋνѵѷүұṽṿⅴ∨ΰϋύὐὑὒὓὔὕὖὗὺύῠῡῢΰῦῧ"},
	{'w',"wŵԝẁẃẅẇẉẘ"},
	{'x',"xϰхҳẋẍⅹ⚔✘✗✖✕"},
	{'y',"yýÿŷƴȳγуўӯӱӳẏẙỳỵỷỹʏᛜᛝ"},
	{'z',"zźżžƶȥʐʑẑẓẕᛉᛦᛧᛨ"},
	{'\0',NULL}
};


/* ---- code starts here ---- */

/* for most of these functions, see the lomoji.h file for API documentation. */

lomoji_ctx_t *lomoji_ctx_new(char **annotations) {
	lomoji_ctx_t *new;

	new = (lomoji_ctx_t*)malloc(sizeof(lomoji_ctx_t));
	new->tts_prefix = g_strdup(DEFAULT_TTS_PREFIX);
	new->tts_suffix = g_strdup(DEFAULT_TTS_SUFFIX);
	new->unknown = g_strdup(DEFAULT_UNKNOWN);
	new->cp_tts = g_hash_table_new_full(g_str_hash, g_str_equal,g_free,g_free);
	new->cp_equiv = g_hash_table_new_full(g_str_hash, g_str_equal,g_free,g_free);
	new->alias_cp = g_tree_new_full(treecompare,NULL,g_free,g_free);

	lomoji_add_equiv(new,NULL);
	lomoji_add_regional(new);
	lomoji_add_oneoffs(new);

	if(annotations) {
		lomoji_add_annotations(new,annotations);
	}

	return(new);
}

void lomoji_ctx_free(lomoji_ctx_t *p) {
	if(!p) return;

	if(p->tts_prefix) g_free(p->tts_prefix);
	if(p->tts_suffix) g_free(p->tts_suffix);
	if(p->unknown) g_free(p->unknown);
	if(p->cp_tts) g_hash_table_destroy(p->cp_tts);
	if(p->cp_equiv) g_hash_table_destroy(p->cp_equiv);
	if(p->alias_cp) g_tree_destroy(p->alias_cp);
	return;
}

gint treecompare(gconstpointer a, gconstpointer b, gpointer user_data) {
	return(strcmp(a,b));
}


static void ann_parser_start_element(
	GMarkupParseContext * context,
	const gchar * element_name,
	const gchar ** attribute_names, 
	const gchar ** attribute_values, 
	gpointer user_data, GError ** error
) {

	struct ann_acc *acc = user_data;

	//fprintf(stderr,"Element <%s>\n",element_name);
	if(!strcmp(element_name,"annotation")) {

		ann_acc_clear(acc);
		for (gsize i = 0; attribute_names[i]; i++) {
			/*fprintf(stderr,"\tattribute: %s = '%s'\n",
				attribute_names[i],
				attribute_values[i]
			);
			*/
			if(!strcmp(attribute_names[i],"cp")) {
				acc->cp = g_strdup(attribute_values[i]);
			}
			if(!strcmp(attribute_names[i],"type")) {
				acc->tts = 1;
			}
		}
	}
}

static void ann_parser_end_element(
	GMarkupParseContext * context, 
	const gchar * element_name, 
	gpointer user_data, 
	GError ** error
) {
	struct ann_acc *acc = user_data;

	//fprintf(stderr,"Element </%s>\n",element_name);
	if(!strcmp(element_name,"annotation")) {

		if(acc->tts) {
			/* this is a canonical tts entry. */
			//fprintf(stderr,"'%s' = '%s'\n",acc->cp,acc->text);
			/* quick pass to turn spaces into underscores. */
			for(gchar *c=acc->text;*c;c++) {
				if(*c==' ') *c='_';
				if(*c==':') *c='_';  /* colons too. */
			}
			/* 
			if((g_hash_table_contains(acc->ctx->cp_tts,acc->cp))) {
				fprintf(stderr,"overriding duplicate tts with %s -> %s\n",acc->cp,acc->text);
			}
			*/
			g_hash_table_insert(acc->ctx->cp_tts,g_strdup(acc->cp),g_str_to_ascii(acc->text,NULL));
			g_tree_insert(acc->ctx->alias_cp,g_str_to_ascii(acc->text,NULL),g_strdup(acc->cp));
		} else if (acc->text) {
			/* this is an alias entry. */
			gchar **aliases;
			aliases = g_strsplit(acc->text,"|",-1);
			for(char **a = aliases; a && *a; a++) {
				char *alias = g_strdup(g_strstrip(*a));
				for(gchar *c=alias;*c;c++) {
					if(*c==' ') *c='_';
					if(*c==':') *c='_';  /* colons too. */
				}
				if(g_tree_lookup(acc->ctx->alias_cp,alias) == NULL) {
					g_tree_insert(acc->ctx->alias_cp,g_str_to_ascii(alias,NULL),g_strdup(acc->cp));
				} else {
					//fprintf(stderr,"skipping duplicate alias %s -> %s\n",alias,acc->cp);
				}
				g_free(alias);
			}
			g_strfreev(aliases);
		}

		/* reset the accumulator. */
		ann_acc_clear(acc);
	}
}

static void ann_parser_characters(
	GMarkupParseContext * context,
	const gchar * text, 
	gsize text_len, 
	gpointer user_data, 
	GError ** error
) {
	struct ann_acc *acc = user_data;

	/* only strdup this text into the accumlator if the cp attribute is
	 * present.  Otherwise, its not text we care about. */
	if(acc->cp) {
		/* the GMarkup docs say text isn't nul-terminated. */
		/* this'll downcase the ascii and alloc at the same time.*/
		gchar *s = g_ascii_strdown(text,text_len);
		acc->text = s;
		//fprintf(stderr,"Text: [%s]\n",nultermd);
	}
}

static void ann_parser_passthrough(
	GMarkupParseContext * context,
	const gchar * passthrough_text, 
	gsize text_len, 
	gpointer user_data, 
	GError ** error
) {
	/*
	char *nultermd = (char *)malloc(text_len+1);
	scnprintf(nultermd,text_len+1,"%s",passthrough_text);
	fprintf(stderr,"passthrough: [%s]\n",nultermd);
	free(nultermd);
	*/
	return;
}

static void ann_parser_error(
	GMarkupParseContext * context, 
	GError * error, 
	gpointer user_data
) {

	fprintf(stderr,"ERROR: %s\n",error->message);

}

void ann_acc_clear(struct ann_acc *acc) {
	if(acc->cp) g_free(acc->cp);
	if(acc->text) g_free(acc->text);
	acc->cp = acc->text = NULL;
	acc->tts = 0;
}

struct ann_acc *ann_acc_new(lomoji_ctx_t *ctx) {
	
	struct ann_acc *new;

	new = (struct ann_acc *)malloc(sizeof(struct ann_acc));
	new->cp = new->text = NULL;
	new->tts = 0;
	new->ctx = ctx;
	return(new);
}

void ann_acc_free(struct ann_acc *acc) {
	ann_acc_clear(acc);
	free(acc);
}


int lomoji_add_annotations(lomoji_ctx_t *ctx, char **annotations) {
	int in;
	char ibuf[8192];
	size_t ilen;
	char *filename;
	int openedok=0;

	const GMarkupParser ann_xml_parser = {
		ann_parser_start_element,
		ann_parser_end_element,
		ann_parser_characters,
		ann_parser_passthrough,
		ann_parser_error
	};

	GMarkupParseContext *context;
	gboolean success = FALSE;

	struct ann_acc *acc;

	/* make sure the ctx pointer isn't null. */
	if(!ctx) return((errno = EPERM));

	for(int f=0;annotations[f];f++) {
		filename = annotations[f];

		/* Can't open the filename?  hummmm..... */
		if((in = open(filename,O_RDONLY))==-1) {
			/* if it doesn't exist, no big deal, but otherwise... */
			if(errno != ENOENT) {
				fprintf(stderr,"Couldn't open '%s': %s\n",
					filename,
					strerror(errno)
				);
			}
			continue;
		}

		openedok++;

		acc = ann_acc_new(ctx);

		context = g_markup_parse_context_new(	
			&ann_xml_parser, G_MARKUP_DEFAULT_FLAGS,acc, NULL
		);

		while( (ilen = read(in,ibuf,sizeof(ibuf))) >0)  {
			success = g_markup_parse_context_parse(context,ibuf,ilen,NULL);
			if(!success) {
				fprintf(stderr,"lomoji: Error parsing '%s'\n",filename);
				break;
			}
		}
	
		/* cleanup. */
		ann_acc_free(acc);
		g_markup_parse_context_free(context);
		close(in);
	}

	/* and if nothing opened ok.. thats a problem. */
	if(openedok == 0) {
		fprintf(stderr,"lomoji: Couldn't open any annotation files, tried ");
		for(int f=0;annotations[f];f++) {
			filename = annotations[f];
			fprintf(stderr,"%s'%s'",
				(f==0)?"":", ",
				filename
			);
		}
		fprintf(stderr,"\n");
		return(ENOENT);
	}

	return(0);
}


/* look for an ascii equivalent char. */
int filter_equiv(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	gchar *sub;
	if( (sub = g_hash_table_lookup(ctx->cp_equiv,check)) ) { 
		(*out) = g_string_append((*out),sub);
		return(1);
	}
	return(0);
}

/* look for a name. */
int filter_toname(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	gchar *sub;
	if( (sub = g_hash_table_lookup(ctx->cp_tts,check)) ) {
		/* a substitution was found. */
		if( (strlen(sub) <= 1) ) {
			/* if the substitution is a single character, don't bother
			 * wrapping it. */
			*out = g_string_append(*out,sub);
			return(1);
		} else {
			*out = g_string_append(*out,ctx->tts_prefix);
			*out = g_string_append(*out,sub);
			*out = g_string_append(*out,ctx->tts_suffix);
			return(1);
		} 
	}
	return(0);
}

/* look for a :emoji: type name, and emit the grapheme if found. */
int filter_fromname(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	GTreeNode *node;
	char *completion;
	gchar *k,*v;
	gchar *keypart;

	/* use the first possible completion. */
	completion = lomoji_suggest_ext(ctx,check,1,NULL);
	if(!completion) {
		return(0);
	}

	keypart = keypart_dup(ctx,completion);
	free(completion);
	if(!keypart) {
		return(0);
	}

	node = g_tree_lower_bound(ctx->alias_cp,keypart);
	if(!node) {
		g_free(keypart);
		return(0);
	}

	k=g_tree_node_key(node);
	v=g_tree_node_value(node);

	if(strncmp(k,keypart,strlen(keypart))!=0) {
		g_free(keypart);
		return(0);
	}

	/* oooh.  It worked! */
	*out = g_string_append(*out,v);
	g_free(keypart);
	return(1);
}


/* translate grapheme into \U+x notation. */
int filter_uplus(lomoji_ctx_t *ctx, gchar *check, GString **out) {
	gchar *start, *end, *cp;
	char buf[1024];
	gunichar c;
	
	for(start = check;*start;start=end) {
		end = g_utf8_find_next_char(start,NULL);
		cp = g_utf8_substring(start,0,
			g_utf8_pointer_to_offset(start,end)
		);
		c = g_utf8_get_char(cp);
		g_snprintf(buf,sizeof(buf),"\\U+%x ",c);
		*out = g_string_append(*out,buf);
		g_free(cp);
	}
	return(1);
}

/* look for an ascii equivalent char. */
int filter_unknown(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	*out = g_string_append(*out,ctx->unknown);
	return(1);
}

/* Try taking the grapheme apart codepoint by codepoint. */
int filter_decompose(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	gchar *start, *end, *cp;
	for(start = check;*start;start=end) {
		end = g_utf8_find_next_char(start,NULL);
		cp = g_utf8_substring(start,0,
			g_utf8_pointer_to_offset(start,end)
		);
		if( filter_toname(ctx,cp,out) == 0 ) {
			filter_unknown(ctx,cp,out);
		}
		g_free(cp);
	}
	return(1);
}

/* use glib's built in inconv based str_to_ascii() */
int filter_iconv(lomoji_ctx_t *ctx, gchar *check, GString **out) {

	gchar *sub;
	sub = g_str_to_ascii(check,NULL);
	*out = g_string_append(*out,sub);
	free(sub);

	return(1);
}

int lomoji_add_equiv(lomoji_ctx_t *ctx, lomoji_equiv_t *source) {

	lomoji_equiv_t *e;
	gchar *start,*end,*cp;
	char ascii[2] = " ";

	/* make sure the ctx pointer isn't null. */
	if(!ctx) return((errno = EPERM));

	if(!source) source=default_equiv;

	for(e=source;e && e->ascii;e++) {
		for(start = e->utf;*start;start = end) {
			end = g_utf8_find_next_char(start,NULL);
			cp = g_utf8_substring(start,0,
				g_utf8_pointer_to_offset(start,end)
			);
			/* useful for finding duplicates in the entries.
			gchar *dup;
			if((dup = (gchar*) g_hash_table_lookup(ctx->cp_equiv,cp))) {
				fprintf(stderr,"Duplicate codepoint: %s was %s\n",cp,dup);
			}
			*/
			*ascii = e->ascii;
			g_hash_table_insert(ctx->cp_equiv,cp,g_strdup(ascii));
		}
	}

	return(0);
}


/* regional indicator characters can be combined to make emoji flags. */
int lomoji_add_regional(lomoji_ctx_t *ctx) {

	char ascii[2] = " ";
	char utf[8] = "";
	char letter;
	gunichar c;

	/* make sure the ctx pointer isn't null. */
	if(!ctx) return((errno = EPERM));

	for(letter='A';letter<='Z';letter++) {
		c = 0x1f1e6 + (letter - 'A');
		*ascii = letter;
		g_unichar_to_utf8(c,utf);
		g_hash_table_insert(ctx->cp_equiv,g_strdup(utf),g_strdup(ascii));
	}
	return(0);
}

void lomoji_add_oneoffs(lomoji_ctx_t *ctx) {

	char utf[8] = "";

	struct pair {
		gunichar cp;
		char *name;
	};
	
	struct pair addthese[] = { 
		/* Note the duplicate entries.  If dup entries exist, user will be able
		 * to enter :zwj: or :with:, but filter will print the last one added-
		 * :with: since "Zero Width Joiner" makes no sense to anyone but
		 * Unicode people. Note also that the dup's can be further overridden
		 * by including language specific redefinitions in an annotations file,
		 * in case you want 'with' to say 'con' or 'zwj' to say 'falegnameria a
		 * larghezza zero'. */
		{ 0x200d, "zwj" },
		{ 0x200d, "with" }, /* dup */
		{ 0xfe00, "vs1" },
		{ 0xfe01, "vs2" },
		{ 0xfe02, "vs3" },
		/* Could include the unused variation selectors here, vs4-vs14, but I'm
		 * not going to. I want them to show up as 'unknown'. -jsj */
		{ 0xfe0e, "vs15" },
		{ 0xfe0e, "text" }, /* dup */
		{ 0xfe0f, "vs16" },
		{ 0xfe0f, "emoji" } /* dup */
	};

	int size = (sizeof(addthese) / sizeof(struct pair));

	for(int i=0;i<size;i++) {
		g_unichar_to_utf8(addthese[i].cp,utf);
		g_hash_table_insert(ctx->cp_tts,g_strdup(utf),g_strdup(addthese[i].name));
		g_tree_insert(ctx->alias_cp,g_strdup(addthese[i].name),g_strdup(utf));
	}

}

char *lomoji_from_ascii_ext(lomoji_ctx_t *ctx, char *src, lomoji_filter **filters) {
	/* scans for tts->prefix'ed words, and runs the filter list against them. */
	gchar *start, *end, *check;
	char *ret;

	/* no source string at all? */
	if(!src) return(strdup("")); 

	/* no ctx?  Return a copy of the orginal string. */
	if(!ctx || !filters) return(strdup(src)); 

	/* ok, start running filters. */
	GString *out = g_string_new("");

	int prefixlen = strlen(ctx->tts_prefix);
	int suffixlen = strlen(ctx->tts_suffix);

	for(start = src;*start;start=end) {

		if(strncmp(start,ctx->tts_prefix,prefixlen) != 0) {
			/* not an ascii representation of an emoji, so just copy it in. */
			out = g_string_append_c(out,*start);
			end = start+1;
		} else {
			/* found what looks like the start of an ascii name for an emoji.
			scan forward looking for tts_sufffix, space, EOS*/
			end = start+prefixlen;
			while(*end) {
				if( (g_unichar_isspace(g_utf8_get_char(end))) ) {
					/* found a space. */
					break;
				} else if ( !strncmp(end,ctx->tts_suffix,suffixlen )) {
					end+=suffixlen;
					break;
				}
				end = g_utf8_find_next_char(end,NULL);
			}
			/* start and end are correct. */
			check = g_utf8_substring(start,0,
				g_utf8_pointer_to_offset(start,end)
			);
			//fprintf(stderr,"I am to check: '%s'\n",check);

			/* loop over the supplied filters until one returns a 1 */
			int submade = 0;
			for(int i=0;filters[i];i++) {
				submade = (filters[i])(ctx,check,&out);
				if(submade) break;
			}
			if(!submade) {
				/* no substitution was made. */
				out = g_string_append(out,check);
			}
			g_free(check);
		}
	}
	/* strdup it instead of g_strdup() so that caller doesn't have to g_free() */
	ret = strdup(out->str);
	g_string_free(out,TRUE);
	return(ret);

}

char *lomoji_to_ascii_ext(lomoji_ctx_t *ctx, char *src, lomoji_filter **filters) {

	gchar *start, *end, *check;
	char *ret;

	/* no source string at all? */
	if(!src) return(strdup("")); 

	/* no ctx?  no filters?  Return a copy of the orginal string. */
	if(!ctx || !filters) return(strdup(src)); 

	/* ok, start running filters. */
	GString *out = g_string_new("");

	for(start = src;*start;start=end) {
		end = g_utf8_find_next_char(start,NULL);
		if( (end-start)==1 && ((*start&0xc0)!=0xc0) ) {
			/* It isn't UTF-8, so just copy it in. */
			out = g_string_append_c(out,*start);
		} else {
			/* keep pulling in zerowidth chars, and next char after that. */
			while(g_unichar_iszerowidth(g_utf8_get_char(end))) {
				end = g_utf8_find_next_char(end,NULL);
				end = g_utf8_find_next_char(end,NULL);
			}
			/* get the codepoint into its own gchar* to check. */
			check = g_utf8_substring(start,0,
				g_utf8_pointer_to_offset(start,end)
			);

			/* loop over the supplied filters until one returns a 1 */
			int submade = 0;
			for(int i=0;filters[i];i++) {
				submade = (filters[i])(ctx,check,&out);
				if(submade) break;
			}
			if(!submade) {
				/* no substitution was made. */
				out = g_string_append(out,check);
			}

			g_free(check);
		}
	}
	/* strdup it instead of g_strdup() so that caller doesn't have to g_free() */
	ret = strdup(out->str);
	g_string_free(out,TRUE);
	return(ret);

}


/* strip the tts_prefix from beginning and optional tss_suffix from the end of
 * input string, returned as a dup.  caller must free the returned string. */
gchar *keypart_dup(lomoji_ctx_t *ctx, char *in) {
	gchar *ret = NULL;
	gchar *stopat = NULL;
	int stop;

	/* suggestion MUST start with the tts_prefix. */
	/* removed if present. */
	int start = strlen(ctx->tts_prefix);
	if(strncmp(in,ctx->tts_prefix,start) !=0)  {
		/* it does not. */
		return(ret);
	}

	/* suggestion MAY end with the tts_suffix. don't include that in the key. */
	if( (stopat = g_strstr_len(in+start,-1,ctx->tts_suffix)) ) {
		stop = stopat - in;
	} else {
		stop = -1;
	}
	

	ret = g_utf8_substring( in,start,stop );
	if(strlen(ret) == 0 ) {
		g_free(ret);
		ret = NULL;
	}
	return(ret);
} 


/* Returns a space separated string containing no more than max possible
 * suggestions.  Returns NULL if no suggestions exist.  Caller must free the
 * returned string. */
char *lomoji_suggest_ext(lomoji_ctx_t *ctx, char *src, int max, int *found) {
	
	GTreeNode *node;
	gchar *k, *keypart;
	int count = 0;
	char *ret = NULL;
	int gotone = 0;
	
	
	GString *out = g_string_new("");

	if(!ctx | !src | !*src) {
		return(ret);
	}

	if (!(keypart = keypart_dup(ctx,src))) {
		return(ret);
	}

	node = g_tree_lower_bound(ctx->alias_cp,keypart);
	while(node && ((max==0)||(count<max))) {
		k = g_tree_node_key(node);
		if( strncmp(k,keypart,strlen(keypart)) != 0) {
			/* src is no longer a prefix of k. */
			break;
		}
		gotone = 1;
		out = g_string_append(out,ctx->tts_prefix);
		out = g_string_append(out,k);
		out = g_string_append(out,ctx->tts_suffix);
		out = g_string_append(out," ");
		node = g_tree_node_next(node);
		count++;
		if(found) (*found)++;
	}
	if(gotone) {	
		ret = strdup(out->str);
	}
	g_free(keypart);
	g_string_free(out,TRUE);
	return(ret);
}

const char *lomoji_get_param_ext(lomoji_ctx_t *ctx, lomoji_param which) {
	if(!ctx) return(NULL);
		
	switch (which) {
		case LOMOJI_PREFIX:
			return(ctx->tts_prefix);
		case LOMOJI_SUFFIX:
			return(ctx->tts_suffix);
		case LOMOJI_UNKNOWN:
			return(ctx->unknown);
		default:
			break;
	}
	return(NULL);
}


const char *lomoji_set_param_ext(lomoji_ctx_t *ctx, lomoji_param which, const char *to) {
	if(!ctx) return(NULL);
	if(!to) return(NULL);
		
	switch (which) {
		case LOMOJI_PREFIX:
			if(ctx->tts_prefix) g_free(ctx->tts_prefix);
			return( (ctx->tts_prefix = g_strdup(to)) );
		case LOMOJI_SUFFIX:
			if(ctx->tts_suffix) g_free(ctx->tts_suffix);
			return( (ctx->tts_suffix = g_strdup(to)) );
		case LOMOJI_UNKNOWN:
			if(ctx->unknown) g_free(ctx->unknown);
			return( (ctx->unknown = g_strdup(to)) );
		default:
			break;
	}
	return(NULL);
}

void lomoji_init(void) {
	errno = 0;
	lomoji_init_filepaths();
	lomoji_default_ctx = lomoji_ctx_new(lomoji_default_filepaths);
}

void lomoji_init_filepaths(void) {

	char filename[PATH_MAX];
	const gchar *homedir;
	int count =0;

	char *files[] = {
		"default.xml",
		"derived.xml",
		"extra.xml"
	};

	if(lomoji_default_filepaths) return;  /* no re-init. */

	lomoji_default_filepaths = (char **)g_malloc(sizeof(gchar *) * ((ARRAY_SIZE(files)*2) +1) );

	for(int f=0;f<ARRAY_SIZE(files);f++) {
		g_snprintf(filename,sizeof(filename),"%s/lomoji/%s",
			SHARE_PREFIX, files[f]
		);
		lomoji_default_filepaths[count++] = g_strdup(filename);
	}

	if( (homedir = g_get_home_dir()) ) {
		for(int f=0;f<ARRAY_SIZE(files);f++) {
			g_snprintf(filename,sizeof(filename),"%s/.lomoji/%s",
				homedir, files[f]
			);
			lomoji_default_filepaths[count++] = g_strdup(filename);
		}
	}
	lomoji_default_filepaths[count++] = NULL;
	return;
}

void lomoji_done_filepaths(void) {

	if(!lomoji_default_filepaths) return;
	
	for(int i=0;lomoji_default_filepaths[i];i++) {
		g_free(lomoji_default_filepaths[i]);
	}
	g_free(lomoji_default_filepaths);
	lomoji_default_filepaths = NULL;
}

void lomoji_done(void) {
	lomoji_done_filepaths();
	if(lomoji_default_ctx) {
		lomoji_ctx_free(lomoji_default_ctx);
	}
}
char *lomoji_to_ascii(char *src) {
	return(lomoji_to_ascii_ext(lomoji_default_ctx,src,lomoji_toascii));
}
char *lomoji_from_ascii(char *src) {
	return(lomoji_from_ascii_ext(lomoji_default_ctx,src,lomoji_fromascii));
}
char *lomoji_suggest(char *src, int max, int *found) {
	return(lomoji_suggest_ext(lomoji_default_ctx,src,max,found));
}

const char *lomoji_get_param(lomoji_param which) {
	return(lomoji_get_param_ext(lomoji_default_ctx,which));
}
const char *lomoji_set_param(lomoji_param which, const char *to) {
	return(lomoji_set_param_ext(lomoji_default_ctx,which,to));
}
