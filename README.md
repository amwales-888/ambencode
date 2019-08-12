# ambencode
Simple Bencode parser in C

From the wikipedia page 'Bencode (pronounced like B-encode) is the encoding used by the peer-to-peer file sharing system BitTorrent for storing and transmitting loosely structured data.'

This Bencode parser is usable with just 2 C files being required ambencode.h and ambencode.c ALL other files are optional. Simply adding the the files to your project and using the three provided APIs.

```
int ambencode_alloc(struct bhandle *bhandle, 
                    struct bobject *ptr, poff_t count);

int ambencode_decode(struct bhandle *bhandle, 
                  char *buf, bsize_t len);
	
void ambencode_free(struct bhandle *bhandle);
```

Once a Bencode buffer has been parsed a DOM is created and can be
manipulated with the provided C Macros.

A number of examples are provided and can be found in the 'examples' 
directory.

The project examples can be built on linux using GNU make and GCC

```
    make
```

The parser is supplied with a example command line utility called
'ambencode' that is generated when you make the examples.

```
    Usage: ./ambencode filepath
           ./ambencode filepath query
           ./ambencode filepath --dump

      filepath      - Path to file or '-' to read from stdin
      query         - Path to Bencode object to display
       	              eg. "uk.people[10].name"
      --dump        - Output minified Bencode representation of data
      --dump-pretty - Output pretty printed Bencode representation of data
      --benchmark   - Output parsing statistics
```
