# Monkdown

Monkdown is an idiosyncratic parser for an ascetic dialect of Markdown.

## Impetus

I realized that I had learned a lot from [my first attempt to parse Markdown](https://github.com/chrischitren/site/tree/old), but that the project was something of a Rube-Goldbergian abomination of ad hoc solutions to ill-defined problems. In the messy process of learning, all in parallel, what *could* be done, what *should* be done, and what *must* be done, I made something that more closely resembled a collage than a tool. This kind of product is a natural consequence of working with constant access to information.

In pursuit of spiritual enlightenment, or perhaps clarity of mind (if there is a distinction between the two), I decided that I had probably learned enough to solve the problem myself. I defined the discipline I would follow:

- I can only search [this C reference manual](https://en.cppreference.com/c).
- I can only write code by hand in a text editor (vim).
- Everything must compile, with no errors, to the standard set by
`gcc -Wpedantic -Wall -std=c89`.

Like all creative works, it is derivative of the things I exposed myself to during the learning phase, but it is not *plagiaristic;* it is wholly my own construction and thus is wholly representative of my capacity to design. The final product is more extensible, coherent, and probably reliable than the spaghetti of my first attempt, but it is still the product of a single fallible human, and is quite myopic in scope. For one, I am chronically bad at error handling. I have too much faith in edge cases being rare. It is not "production" code because it is not robust or well-tested, and it does not conform to a well-defined specification. It is art code.

> The universe (which others call the Library) is composed of an indefinite, perhaps infinite number of hexagonal galleries.
>> Borges, *The Library of Babel*

## Nodes

The primary struct that allows me to maintain clarity is the Node. Nodes have parents and children and are linked to form a document tree. The fields of a Node that correspond to document content, as well as its list of children, are dynamically allocated to reduce memory usage. A Node has the following fields:

```
struct node {
	Node *parent;
	Node **children;
	int numchildren;
	char *content;
	char *attributes;
	int contentlen;
	int type;
	int isterminal;
};
```

Since `content` and `attributes` are malloc'd strings, they are mutable, and can be manipulated as needed while parsing and writing. This removes a lot of processing overhead. In my first attempt, I was intent on storing as little as possible in each node of the tree, instead choosing to store positions and run-lengths in the source file; a consequence of this decision was that manipulations of the document tree could only happen at the moment of writing, or by implementation of some new flag or indicator stored in each node. For example, I want to change links to `.md` files to be links to `.html` ones. In the new framework, this is a trivial find-and-replace operation that can happen at any time. In my first attempt, it *had* to be done at the moment of writing because the nodes themselves could only reference the source. The code for writing outputs became cumbersome and tangled with context-aware functions to handle all sorts of special cases.

Ideally, the Parser does not "know" that its outputs will eventually become HTML. It "understands" a grammar that is a definition of the mapping between source strings and document trees. Similarly, the Writer does not "know" that the document tree was built by parsing markdown; it wants Nodes that possess metadata and contents, and it makes decisions about what HTML tags should be used based on the metadata, mutates Node contents to its liking, and writes. If we wanted to handle another structured format for representing text with links and styles, the Parser should not have to be modified to accommodate it; a new Writer should be written that takes the same internal, "complete" representation of whatever ideas Markdown is capable of expressing, and maps it to the specification of the new structured format.

## Parsing

The parser works in two stages: a bird's-eye-view separation of the document into contiguous **blocks**, and **inline parsing** of the text content of blocks. Blocks correspond to `<pre>` (code blocks), `<ul>` (unordered lists), `<h1>` and co. (headings), and plain text.

### Blocks

Blocks cannot be nested. This is a feature of Markdown that has been omitted here. A blockquote cannot reside inside a list item. I am not too concerned about this because I have only rarely seen it used, and because I have never wanted such a feature. The idea has a formal beauty: that Nodes, agnostic to their positions, are atomic, and that documents can be infinitely nested and arbitrarily structured. This beauty, however, is at odds with what Markdown's features suggest a document's structure should be: headings define sections that contain contents, and contents are like those of printed matter. If I wanted to express something outside of these constraints, I would use a different tool. Like a pencil.

- Blocks are delimited by blank lines, except for code blocks, which must start and end with three backticks.
- Blocks are identified by their first character(s). This one, for example, is an unordered list because it starts with a hyphen.
- Blocks are stripped of their control characters so that the rest of the document can be parsed inline.
- Blocks are "easy" to add more of, if this is needed. Not "easy" in the sense of a configuration file with a simple human-readable syntax, but easy in that the block-level parsing functions just need some new cases. Hackable.

### Inline Elements

Each inline style is requests two functions:

- `scan` functions return run-lengths by looking for control characters.
- `split` functions build Nodes based on strings that were extracted by `scan` functions.

In a beautiful implementation, the `scan` functions would be regular expressions.

All nodes which are not terminal are scanned and lists of scan results are produced. Results may not overlap. Nodes have their contents distributed among newly-created children: scan-result-Nodes interleaved with unformatted-content-Nodes. Scan-result-Nodes go through 'split' functions, and unformatted-content-Nodes are simply appended as terminal plaintext Nodes. Split functions decide whether the Nodes they produce should be terminal.

The process is repeated until either a predefined limit is reached, or every Node without children `isterminal`. A better term for this would have been `isfrozen`, since "terminal" has established meaning in the context of trees.

## Writing

Writing is easy. The document tree is walked, depth-first, and tags are printed according to Nodes' types. If a Node has an `attributes` string, it is printed within the tag. Nodes' contents are printed. All the Node's children are given the same treatment, and then the matching closing tag is printed (unless it's an `<img>`).

We did the hard work when we built the tree intelligently, so the structure of the document tree aligns well with the syntax of HTML.

