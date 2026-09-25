# Personal Website & Portfolio

This is a repo to store the markdown sources for my personal site. I don't really know what I'm doing, but I'm trying to understand the process as a whole, so it might not work or it might be interesting.

Inspired by lots of sites on the [xxiivv webring](https://webring.xxiivv.com/), especially those of [Devine Lu Linvega](https://wiki.xxiivv.com/) and [Rek Bell](https://kokorobot.ca/). I wanted somewhere to link to others' work, act as a collection of my thoughts, showcase projects or creations, you get it. A million people have done it.

Right now I think that there are three parts to this project:

- A directory of markdown files, attachments, etc. which feeds into
- a static site generator, to produce
- a directory of html and css files which comprise the actual site.

This third stage can be handed to Cloudflare, Github, or the like, who can hopefully handle the enormous complexities of infrastructure.

## The Importance of Thinking

I have been grappling with this site for a while. Its growth has been partially stunted by my ignorance of C, but this is a mere challenge of comprehension. C already exists and is well documented, so it can be learned to any degree seen fit by anyone with the time and will to do so. The process that has really impeded progress is nailing down how the site ought to be shaped.

There are some stock ways to construct a collection of webpages. A chronology is an obvious choice for something like a blog, where the style of content is imitative of a journal or a log. A particularly ascetic example of a framework to accomplish this is [Bear Blog](https://bearblog.dev/). Perhaps the antithesis of this is a mess-of-pages approach, where everything is dumped into a folder and a human brain sorts out the details, (ostensibly) ensuring that links actually lead somewhere and that the thing is navigable by anyone other than its creator. This is what tools like [Obsidian](https://obsidian.md/) produce in the absence of master planning by the user. It is a compelling format because it mirrors the structure of the Internet itself; pages are linked, and that's that.

These approaches are not mutually incompatible. A chronology can be a mess-of-pages internally, so long as each page is timestamped so that an index can be generated.

### Engineering 1: Despaghettification

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

We see that our struct contains insufficient information! We must also know *where* in the parent element the new element should be inserted. (While we're editing, 256 bytes of attributes is wild for a few reasons: first, most elements have no attributes; second, our whole struct only takes up 1620 bytes; third, the URLs we need for the most common kind of attribute (`href`) are already present in the markdown source!)

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


```
/*
	The very VERY first thing we must do is find the code blocks. They are
	not strictly delimited by blank lines like all other element types.

	The very first (next) thing we must do is split the source into chunks
	delimited by blank lines. Each of these is assigned a tag based on its
	first (nonspace) character: 

	#       Heading
	>       Block quote
	!       Image
	-       Unordered list

	If the first (nonspace) character is not a control character, the chunk
	is a <p> element alone. 
	
	These chunks are stored in a list, or perhaps processed sequentially.

	The whole chunk is read into memory so we can treat it like a string.
	This is to reduce our reading and writing time.
*/
```

### Engineering 4: Reevaluating Our Decisions

Our goal is to read a markdown file and generate an HTML document. To do this, we must specify what we think "markdown" is. We are coming up with our own tiny flavor here, and we have the requirement that its features are a subset of "standard" markdown's.

Elements are nodes of a tree. The root of this tree is the Document. The following elements are required:

```
Document
|
+- Body
   |
   +- Main
```

### Engineering 5: `***Asterisks:* strong & em**`

A collection of verbose comments that were cluttering up `builder.c`:

```
/*
    FIRST PASS CHUNKS:

             Standard:      <p>
                                text
                            </p>

    >        Blockquote:    <blockquote>
                                text
                            </blockquote>

    -        List:          <ul>
                                <li>text</li>
                            </ul>

    1.       Ordered list:  <ol>
                                <li>text</li>
                            </ol>

    #        Heading:       <h1>text</h1>
                            ...
                            <h6>text</h6>

    !        Image:         <a href="link"><img src="link" name="text"></a>

    ```      Code block:    <pre>
    text                        text
    ```                     </pre>

*/
```

```
/*
	After parsing the first pass, we will not have a tree. We will have a
	list. Chunks are found using the following algorithm:

	FIND CHUNKS:
		Start at the beginning of the document.
		Skip to the first character that is not a newline or space. 
			Are the next three characters exactly "```"? Yes:
				Mark the beginning of a code block.
				Skip to the next occurrence of the sequence "```\n".
					If the end of the document is reached, end the block.
				Mark the end of the code block.
			No:
				Mark the beginning of a block.
				Identify the block based on	its first character.
				Skip to the next occurrence of the sequence "\n\n".
					If the end of the document is reached, end the block.
				Mark the end of the block.
		Is it the end of the document? No:
			GOTO "Skip to the first character that is not a newline or space"
		Yes:
			END
*/
```

```
/*
	
	We now have a list of chunks that are contiguous regions of similarly-
	formatted content. They are useful because they allow us to prune the
	transition table; for example, we will never go from being in a heading
	to being in a list element. We can define a recursive routine to parse
	inline text, which contains only literal characters & control sequences
	with comprehensible nesting rules.

	The inline parser can "eat" symbols greedily. Take the example:
	
		Styles draw the ***eye.*** This [*italic* is in a link](/link). This
		*[link](/link)* is in an italic.

	Our inline parser is a simple state machine. It knows the context of the
	chunk it is in, and has a transition table based on this context, the
	current state, and the next character. 

	Our inline parser is a recursive function. When it consumes a character,
	it may create a new instance of itself with context informed by its own.
	The things it can do are:

		Consume a character and do nothing.
		                        append it to self->content.
		                        append it to self->attributes.
		                        give birth.
		                        ascend.

*/
```

```
/*
	We need to define a way to allow or disallow recursion based on the
	current context of parseinline. We need to check the next two characters
	or else we cannot properly parse ***text*** and [links](link_url).
	
	The thing to do is to break the text into iElements containing substrings
	of the original. To do this we loop over the entire thing looking for
	the delimiters (in order of precedence):

         0123         0123        01         012        01         01
            .            .         .           .         .          .
	    \n-...\n+    \n>...\n+    `+...`+    **...**    *+...*+    [+...)+
	    li           p            code       strong     em         a
	    ^ul          ^blockquote

	where + indicates that any character matches. It is important to check for
	** before * because we don't want to wind up with three nested <em> tags
	where there should be a <strong><em>...</em></strong>. 

	There is no valid markdown that generates <a><li>text</li></a>, and
	similarly our parser should *not* generate <code><em>text</em></code>.
	Thus we must decide which elements can be matched inside which others.

	         inside this,      
	         li p  cd st em a  
	 a    li -- -- -- -- -- -- 
     l t   p -- -- -- -- -- -- 
     l h  cd YY YY -- YY YY YY 
     o i  st YY YY -- -- -- YY 
     w s  em YY YY -- YY -- YY 
       ?   a YY YY -- YY YY -- 

	UNFORTUNATELY this nice little table is insufficient to parse some of the
	more obtuse parts of the syntax, at least as far as I can see from my
	attempts to hack it into order. 
	
	Hitting a delimiter will need to call a function (of bespoke design) that
	inspects the rest of the chunk to find the end of the sequence. Some of
	these will be very simple functions, like for code blocks: check for the
	next '`' that isn't escaped. The one for em/strong is less pretty.

	These functions should return a few values: a length (or error code, -1?),
	whether the inner string is frozen (contains no further unescaped control 
	characters), an element type... maybe they should just construct the inner
	string themselves and pass back a pointer to a parent-linked iElement?

	The great value of just returning positions and line lengths is avoiding
	ever allocating (especially in the heap) short-lived buffers. All we do is
	read, decide, and return an instruction to the primary "read-write head"
	that has a preloaded copy of the chunk into reference quickly.

	As written, parseasterisk gobbles up MAX_CHUNK_SZ bytes immediately, and
	any other function will also need to do this. If they just had to read
	bytes from a pointer+offset and make a stop/continue decision at each char
	based on a small internal state, they would (hopefully) be more efficient.
	
	Of course, these functions don't get called on every character. It would
	probably be fine if they needed to skim a little stack...
*/
```

```
/*
	next time you find a run longer than the current depth, pop up to toplevel,
	but do so at the *end* of this new run. e.g:
	
		*italic **bold text***
			-> <em>italic <strong>bold text</strong></em>
		
		*italic**bold text***
			-> <em>italic<strong>bold text</strong></em>
			-> <em>italic</em><em>bold text</em></strong>
	
	Which parse is "correct"? As a browser, we know that the second HTML is
	invalid. As backtrackers, we know that there was an alternative parse of
	the markdown that would have produced valid HTML.
	
	The simple solution is to only allow * to be ended by * or ***, and if the
	latter, then we must "consume" the final asterisk to ensure that subparses
	are successful. Similarly, ** can only be ended by ** or ***, and we must
	consume the end of the triplet to leave inner strings well-formed. 
	
		+- consumed         +- consumed
		|                   |
		*italic**bold text***
		|      |          |
		|      |          +- see triplet; can end; skip ahead by 3-depth
		|      |
		|      +- cannot end depth 1 with runlength 2
		|
		+- start with depth 1
	
	The next problem is starting at depth 3. We cannot know whether we should
	be in a 1-depth or 2-depth on the first pass, so we cannot know how many
	characters to consume.
	
	We can hack it in the following way:
	
		Set the depth to 3 and consume {all 3 asterisks, no asterisks}
		As we read other runs, count down the depth.
		When the depth reaches zero, consume the run that did so;
			calculate what should have been consumed at the start;	
			prepend asterisks to correct the inner string.
	
	This sucks because we have to backtrack (sort of; we can write,
	
	    ***text with** italic*
	    ^^^^^^^^^^^^^^^^^^^^^
	
	to the buffer, compute that we should have consumed one, and then pass
	*(buffer+1) as the inner string), but I don't think it's possible to avoid
	in this case.
*/
```

```
/*	BROKEN (BUT THE GIST IS THERE) PSEUDOCODE FOR PARSEASTERISK

	INSIDE = 0;
	N = 0;
	START_N = 0;
	DEPTH = 0;
	LAST = 0;
	
	if you match,
		N = [length of match]
		and !INSIDE,
			START_N = N
			DEPTH = N
			and N < 3,
				skip START_N
				TAG <- START_N
			and N == 3,
				write "***" to buffer
				skip START_N
		and INSIDE,
			and START_N < 3,
				and N == START_N,
					write *(content)
					skip START_N
				and N == 3,
					and DEPTH == 3,
						skip 3-START_N
						write *(content)
						skip START_N
					AND DEPTH < 3,
						skip START_N
						write *(content)
				and N < 3 && N != START_N,
					DEPTH += N
					skip N
			and START_N == 3,
				and N == 3,
					skip 1
					TAG <- 1
					write *(content+1)
					skip 2
				and N < 3,
					and DEPTH + N >= 3,
						TAG <- LAST
						write *(content+LAST)
						skip LAST
					and DEPTH + N < 3,
						LAST = N
						DEPTH += N
						skip N
*/
```

The most important discovery was a way to handle the dual meaning of `*`. The last comment above is incomplete and broken, but in `builder.c` the function `parseasterisk` is its working implementation. The working principle is that we wait for runs of asterisks and count their lengths (up to three sequential asterisks). The state of the machine is the current depth of emphasis (1, 2, or 3) and the length of the starting run. Strings that start with 3 asterisks need to be handled very differently from those that start with 1 or 2.

I won't explain all the logic here. What I am really concerned about is whether there are any cases that have a valid way to be mapped to HTML, but for which my logic fails. I also care if there are malformed strings that produce catastrophic errors rather than just undefined behavior. 

## The Importance of Doing

Okay, I have written a few hundred lines of slightly less mangled code to produce trees out of markdown sections. I call these sections **chunks**: they are of a contiguous "type" and are separated by blank lines. By "of a contiguous type," I mean that all of their content can be wrapped in a single HTML tag (like `<blockquote>` or `<ul>`) without any children "not fitting in" (you would not expect an `<li>` to be a direct descendent of a `<blockquote>`).

What I now require is a way to get all the chunks out of a file smoothly and quickly. I am thinking that I will read N characters into a buffer, look for all its chunks, and, if we hit the end of the buffer before the end of a chunk, we will read from the beginning of the chunk.
