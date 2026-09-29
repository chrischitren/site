# The Librarian

The `librarian` handles the interface between the filesystem and the program's data structures. **Its work consists of five sequential tasks:**

## 1. Indexing

The `librarian` walks a given directory recursively in search of source files. Along the way, it generates an index of the paths (relative to the program's working directory) of these source files.

### Subtasks: Flattening & Renaming

The source files have extensions like `.md` and relative paths like `src/topic1/file1.md`. These are good for organizing your thoughts, but the source directory tree has no bearing on the link structure of the resulting site. The source files must be associated with destination filenames in a flat directory:

```
src/topic1/file1.md          ->  out/file1.html
src/index.md                 ->  out/index.html
src/topic2/file2.md          ->  out/file2.html
images/dog.png               ->  out/dog.png
src/project1/images/cat.png  ->  out/cat.png
```

The rationale behind this flattening is that we can avoid "losing" files if we decide to move or reorganize the source material. The **filenames are unique identifiers** that must be chosen wisely (or that must at least persist between updates), but when `images/dog.png` becomes `attachments/dog.png`, you dont have to go on a wild goose chase to find and every instance of `images/` (and make sure it's the *right* `images/`, since there's nothing stopping you from having non-sibling directories with identical names).

This is not to say that the user needs to preempt the flattening. You can be as rigorous or as lackadaisical as you wish about keeping your directories all matched up with reality in your internal links, but all the site will care about is whether the names match.

## 2. Calling the Parser

Now that we know where all our source material resides, we can make something structured of it. We wrote some (rather bad) routines to parse files into `ChunkElement` trees and collate those trees' root nodes into `Document` structs. What the `librarian` really wants is a black box that converts Markdown into HTML, and that's basically what I've given it, but with some dodgy pointer arithmetic and a poor dependency structure to work around. (I would call the dialect of Markdown it recognizes `monkdown`, since it is ascetic, isolated, and makes a lot of *a priori* judgements about what parts of Markdown are important.)

Regardless of the parser's idiosyncrasies, the next task will require that we find all the documents' links. The parser can help us with this by building a list of links as it encounters them while growing its trees (smart, efficient, elegant), or we can do a slapdash search through the entire `Document` right at the end because we didn't plan very well (we can call this something fancy-sounding, like "strongly layered abstraction", and claim that it's important to make sure the parser only has one job).

## 3. Validating Link Structure

Now that we've acquired a list of all the links in each file, we just need to make a matrix! The element `linkmatrix[i][j]` corresponds to whether the file with index `i` links to the file with index `j`, being one if it does and zero otherwise. 

## 4. Building Navigation Tables



## 5. Calling the Scribe

Again, we have to unfortunately come face-to-face with 
