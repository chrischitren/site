# Personal Website & Portfolio

This is a repo to store the markdown sources for my personal site. I don't really know what I'm doing, but I'm trying to understand the process as a whole, so it might not work or it might be interesting.

Inspired by lots of sites on the [xxiivv webring](https://webring.xxiivv.com), especially those of [Devine Lu Linvega](https://wiki.xxiivv.com/) and [Rek Bell](https://kokorobot.ca/). I wanted somewhere to link to others' work, act as a collection of my thoughts, showcase projects or creations, you get it. A million people have done it.

Right now I think that there are three parts to this project:

1. A directory of markdown files, attachments, etc. which feeds into
2. a static site generator, to produce
3. a directory of html and css files which comprise the actual site.

This third stage can be handed to Cloudflare, Github, or the like, who can hopefully handle the enormous complexities of infrastructure.

### The Importance of Thinking

I have been grappling with this site for a while. Its growth has been partially stunted by my ignorance of C, but this is a mere challenge of comprehension. C already exists and is well documented, so it can be learned to any degree seen fit by anyone with the time and will to do so. The process that has really impeded progress is nailing down how the site ought to be shaped.

There are some stock ways to construct a collection of webpages. A chronology is an obvious choice for something like a blog, where the style of content is imitative of a journal or a log. A particularly ascetic example of a framework to accomplish this is [Bear Blog](https://bearblog.dev/). Perhaps the antithesis of this is a mess-of-pages approach, where everything is dumped into a folder and a human brain sorts out the details, (ostensibly) ensuring that links actually lead somewhere and that the thing is navigable by anyone other than its creator. This is what tools like [Obsidian](https://obsidian.md/) produce in the absence of master planning by the user. It is a compelling format because it mirrors the structure of the Internet itself; pages are linked, and that's that.

These approaches are not mutually incompatible. A chronology can be a mess-of-pages internally, so long as each page is timestamped so that an index can be generated.

### Engineering

Okay, I have written 500 lines of spaghetti and only now am I beginning to understand how the thing should work. It needs two passes. The first parses to a lightweight *data structure* that is stored in RAM. This structure is a tree. (The educated reader may now laugh at how long it took me to realize that building a tree is an important step in building an HTML file.) The second pass walks along the tree, whose nodes contain pointers to the relevant content in the source files, and assembles an HTML file to a stream.

Every node needs a pointer to its parent and an array of pointers to its children. It needs a string corresponding to the tag and one corresponding to the attributes thereof. It needs content to go inside the element (this should be the only string that is sanitized). 

```
struct element {
	struct element *parent;
	struct element *children[MAX_CHILDREN];
	char *tag;
	char *attributes;
	char *content;
/*	size_t contentlen; perhaps? */
}
```

I want to avoid loading the whole file into memory. I already have an index of all the source files and they should not change while the program is running, so I can just store file positions returned by `ftell`:

```
struct element {                       /****** SIZE JUSTIFICATIONS ******/
                                       /*      -------------------      */
	struct element *parent;            /*  struct reproduces asexually  */
	struct element *children[128];     /*  how long is a page, really?  */
	char tag[64];                      /*  longest tag is "blockquote"  */
	char attributes[256];              /*  might hold a url or whatnot  */
	long poscontent;                   /*  only files <9 quintillion B  */
	int lencontent;                    /*  no run-on sentences allowed  */
}
```

Assuming a pointer is 8 bytes, this sums up to 8+1024+64+256+8+4=1620 bytes for each element, which is very reasonable! I counted a sample page I made and its most fecund element (`<main>`) has 23 children. The whole document has around 80 elements, so a particularly rambling page might have 500 or so. This is 810 kB to store in the format outlined above.

Let's assemble an HTML document with these structures. Take the sample markdown:

```
Hello, [*italic* world!](link_url)

Line
```

This should become:

```
<main><p>Hello, <a href="url"><em>italic</em> world!</a></p><p>Line</p></main>
```

If we indent the elements to see the tree structure:

```
0         <main>                          
0 1           <p>Hello, 
0 1 2             <a href="link_url">
0 1 2 3               <em>
0 1 2 3                   italic
0 1 2 3               </em>
0 1 2              world!
0 1 2             </a>
0 1           </p>
0 4           <p>
0 4               Line
0 4           </p>
0         </main>
``` 

```
0 main       
|
+- 1 p
|  |
|  +- 2 a
|     |
|     +- 3 em
|
+- 4
```

We see that our struct contains insufficient information! We must also know *where* in the parent element the new element should be inserted. (While we're editing, 256 bytes of attributes is wild for a few reasons: first, most elements have no attributes; second, our whole struct only takes up 1620 bytes; third, the URLs we need for the most common kind common kind of attribute (`href`) are already present in the markdown source!)

We could indicate in each element whether it should open the html element, close it, or both; every time a new subelement is reached while building the tree, its parent is never modified again, the completion of the element is entrusted to the creation of a next sibling. **This is the wrong approach.** It destroys two of the great values of the tree structure: (1) If one node contains malformed data and can't be parsed, we can simply prune the branch, throw an error, and get on with our lives. We don't have to worry about searching the tree for the relevant siblings; (2) The nesting of the elements represented by their pointers to each other can exactly match the nesting of HTML elements. If we allowed opening- and closing-only elements, we could open a tag on one level and close it at a different level (or never, or any number of problematic things).

Anyway, now that we've talked ourselves out of that blunder, what is a *good* approach?

We already know what kind of elements we need to include. We could write format strings with placeholders that are filled by text grabbed from the source file.

Inserting inline elements like `<em>` or `<code>`:

1. We need to know where these elements should be inserted into their parent's content.
2. We must decide how many inline elements might be the child of a single element (we already have, 128).

It occurs to me that *every* child needs to state where it gets inserted into its parent's content, but most are agnostic to the details as long as the order of children is maintained, requesting (implicitly) nothing more than to be appended to the end of their parents contents as generated thus far.

```
00000000000111111111222222222233333333334444444444555555555566666666667777
01234567890123456789012345678901234567890123456789012345678901234567890123

markdown is *quite* complicated! here is a [spec](https://commonmark.org/)
|           |                              |
|           |                              +- "a", start=43, end=73,
|           |                                      startc=44, endc=47,
|           |                                      starta=50, enda=72
|           |
|           +- "em", start=12, end=18,
|                    startc=13, endc=17,
|                    starta=-1, enda=-1
|
+- "p", start=0, end=73,
        startc=0, endc=73,
		starta=-1, enda=-1              
```

```
writetofile(element root, bool recursive):
	LOAD immediate children
	COMPUTE which children's contents lie within mine
	COMPUTE strings tag(type, attributes), endtag(type)
	WRITE tag

	if children == 0:
		for char in [startc, endc]:
			WRITE escaped(char)
		WRITE endtag
	else: 
		prevend = start
		for each child:
			for char in [prevend, child.start-1]:
				WRITE escaped(char)
			prevend = child.end
			if recursive:
				writetofile(child)
			else:
				WRITE child.id
		WRITE endtag
```

### Engineering 2: Intro to Reading

Unfortunately, these elements don't grow on trees (hehe). We'll have to actually build them by reading the markdown, the logic of which we haven't given a ton of thought. We can skip over arbitrary chunks of the file when we reach an element's child, but when should we make new elements? How can we ensure that malformed markdown doesn't produce a malformed tree? My basic idea is that certain sequences of characters start and end every element, but it's difficult to formalize this notion because the sequences follow a complex syntax. Take a heading:

```
\n
### This is a heading.\n
\n
Content under the heading.\n
\n
- List element 1\n
- List element 2\n
\n
More content.\n
```

All headings start with `{'\n', '#'}`, in that order, except for those on the first line of the file. Possible workarounds are to disallow headings at the start of a file, write special case handling for this, or find another way to identify a heading. I think the first is initially the most appealing, since this would be a good place to put non-printing directives like tags, categories, etc. (These should really be generated dynamically from the content of the page, to avoid sloppiness in the structure of the website.)

Block elements start with `{'\n', '-'}` and `{'\n', '>'}`, unless it is a code block, which starts with `{'\`', '\`', '\`'}`. This last one is an exception, but it's a welcome one, since, as a user of markdown, we enjoy not having to prepend special sequences to all the lines of pasted code blocks (and what if these special sequences look similar to the syntax of the code you're pasting? This would introduce a lot of room for error).

The most desirable solution (to me) is a kind of control-sequence-based pushing-and-popping into and out of tree nodes. This is exactly what HTML is, and it's exactly what I want markdown to be.

```
	/*  
	BASIC CONTROL FLOW

	when we get born:
		LOG START
		LOG STARTC
	LOOP: is it the end of the document?
		do we recognize a control sequence? yes:
			are we escaped? no:
				is it one that modifies our current context? yes:
					by ending it? yes:
						LOG ENDC
						(LOG ENDA)
						LOG END
						INCREMENT
						ASCEND
					no:
						LOG ENDC
						LOG STARTA
				no:
					are we allowed to recurse in this context? yes:
						DESCEND
					no:
						SKIP
			yes:
				SKIP
		no:
			SKIP
		INCREMENT
	LOG ENDC
	LOG END
	ERROR because we didn't wind up back at parent
	EXPLODE!


	What are these control sequences?
		DSH '-'  DaSH
		SPC ' '  SPaCe
		NLN '\n' NewLiNe
		BTK '`'  BackTicK
		SBO '['  Square Bracket Open
		SBC ']'  Square Bracket Close
		PBO '('  Paren Bracket Open
		PBC ')'  Paren Bracket Close
		ABO '<'  Angle Bracket Open
		ABC '>'  Angle Bracket Close
		EXC '!'  EXClamation
		HSH '#'  HaSH
        1,6      integer from {1,2,3,4,5,6}

		_0_ _1_ _2_    ELEMENT START /END (MOD)     INLINE?
		-----------    ------------------------     -------
		NLN NLN DSH    ul, li                           
		NLN DSH SPC    li /li                           
		BTK ___ ___    code /code                    yes
		AST AST ___    strong /strong                yes
		AST ___ ___    em /em                        yes
        SBO ___ ___    a(inner)                      yes
        SBC PBO ___    a(href)                       yes
        PBC ___ ___    /a                               
        BTK BTK NLN    pre                              
        NLN BTK BTK    /pre                             
        NLN NLN ___    p /p /ul /blockquote             
        NLN NLN ABC    blockquote                       
        NLN EXC SBO    img... erm... awkward!           
        NLN HSH 1,6    wuh? guh?                        
        ___ ___ ___                                     
        ___ ___ ___                                     
        ___ ___ ___                                     


	This turns out to be pretty complicated. Do there exist control
	sequences such that each sequence corresponds to one operation, 
	potentially querying the state of the machine but not requiring 
	reading outside of the three-char window, so as to produce valid
	html trees? A further requirement is that the syntax this system
	is designed to parse is itself a strict subset of markdown, so 
	that any well-formed file under my syntax is also a well-formed
	markdown file?

	Allowing the system to "potentially query" an arbitrary "state"
	makes this pretty meaningless. What kind of decision making can
	we subsequently employ? Too many state variables means we might
	as well just be reading other characters out of the file.
*/
```

Once again, we are probably trying to hammer markdown into a shape it doesn't fit comfortably in. We ran into this issue when naively attempting to translate based on spaghetti-string-operations born from an intuitive notion of what markdown strings map to which HTML strings, and we solved it (for the output) by realizing that we were trying to create a particularly structured form of data. The markdown itself is also a "particularly structured form of data," but it isn't as simple a structure as a tree. It looks like some kind of syntax, but it isn't one that maps cleanly to a tree. Even though it ought to be.

What are the "rules of markdown," as I conceive of them?

Outside of a code block:

- A blank line returns to the root.
- A newline ends the current element (but does not necessarily return to the root)

### Engineering 3: Grammar

 
