//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. 107403 Name: João Luís
//   N.Mec. 107457 Name: Diana Miranda
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadh_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
//
// static configuration
//

#define _max_word_size_ 32

//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;    // link to the next adjacency list node
  hash_table_node_t *vertex; // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_]; // the word
  hash_table_node_t *next;    // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;      // head of the linked list of adjancency edges
  int visited;                 // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous; // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
  hash_table_node_t *last_word;      // last word of the conected component
};

struct hash_table_s
{
  unsigned int hash_table_size;   // the size of the hash table array
  unsigned int number_of_entries; // the number of entries in the hash table
  unsigned int number_of_edges;   // number of edges (for information purposes only)
  hash_table_node_t **heads;      // the heads of the linked lists
};

//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if (node == NULL)
  {
    fprintf(stderr, "allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if (node == NULL)
  {
    fprintf(stderr, "allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}

//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if (table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i, j;

    for (i = 0u; i < 256u; i++)
      for (table[i] = i, j = 0u; j < 8u; j++)
        if (table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while (*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc; // depois é preciso dividir pelo hash_table_size
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if (hash_table == NULL)
  {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }

  hash_table->hash_table_size = 2000;
  hash_table->number_of_entries = 0;
  hash_table->number_of_edges = 0;
  hash_table->heads = (hash_table_node_t **)malloc((size_t)hash_table->hash_table_size * sizeof(hash_table_node_t *));
  if (hash_table->heads == NULL)
  {
    fprintf(stderr, "allocate hash_table->heads: out of memory\n");
    exit(1);
  }

  for (i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;

  return hash_table;
}

static void hash_table_grow(hash_table_t *hash_table)
{

  hash_table_node_t **old_heads = hash_table->heads;
  unsigned int old_hash_table_size = hash_table->hash_table_size;
  unsigned int i;

  hash_table->hash_table_size = hash_table->hash_table_size * 2;
  hash_table->heads = (hash_table_node_t **)malloc((size_t)hash_table->hash_table_size * sizeof(hash_table_node_t *));

  if (hash_table->heads == NULL)
  {
    fprintf(stderr, "allocate hash_table->heads: out of memory\n");
    exit(1);
  }

  for (i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;

  for (i = 0u; i < old_hash_table_size; i++)
  {
    hash_table_node_t *node = old_heads[i];
    while (node != NULL)
    {
      hash_table_node_t *next = node->next;
      unsigned int j = crc32(node->word) % hash_table->hash_table_size;
      node->next = hash_table->heads[j];
      hash_table->heads[j] = node;
      node = next;
    }
  }

  printf("hash_table_grow: old hash table size = %u new hash table size = %u\n", old_hash_table_size, hash_table->hash_table_size);

  free(old_heads);
}

static void hash_table_free(hash_table_t *hash_table)
{

  unsigned int i;
  hash_table_node_t *node;
  hash_table_node_t *next;
  adjacency_node_t *adjacency_node;
  adjacency_node_t *next_adjacency_node;

  for (i = 0u; i < hash_table->hash_table_size; i++)
  {
    node = hash_table->heads[i];
    while (node != NULL)
    {
      next = node->next;
      adjacency_node = node->head;

      while (adjacency_node != NULL)
      {
        next_adjacency_node = adjacency_node->next;
        free_adjacency_node(adjacency_node);
        adjacency_node = next_adjacency_node;
      }

      free_hash_table_node(node);
      node = next;
    }
  }

  free(hash_table->heads);
  free(hash_table);
}

static hash_table_node_t *find_word(hash_table_t *hash_table, const char *word, int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;

  if (insert_if_not_found)
  {

    node = allocate_hash_table_node();
    strcpy(node->word, word);

    if (hash_table->number_of_entries > hash_table->hash_table_size)
      hash_table_grow(hash_table);

    node->next = hash_table->heads[i];
    hash_table->heads[i] = node;

    hash_table->number_of_entries++;
    node->representative = node;
    node->number_of_vertices = 1;
    node->visited = 0;
    node->head = NULL;
    node->last_word = NULL;
  }
  else
  {
    node = hash_table->heads[i];
    while (node != NULL)
    {
      if (strcmp(node->word, word) == 0)
      {
        return node;
      }
      node = node->next;
    }
  }

  return NULL;
}

// Acrescentei estas funções---------------------------------------------------

void print_table(hash_table_t *hash_table)
{
  for (unsigned int i = 0; i < hash_table->hash_table_size; i++)
  {
    if (hash_table->heads[i] == NULL)
    {
      printf("\t%i\t---\n", i);
    }
    else
    {
      printf("\t%i\t", i);
      hash_table_node_t *tmp = hash_table->heads[i];
      while (tmp != NULL)
      {
        printf("%s ->", tmp->word);
        tmp = tmp->next;
      }
      printf("\n");
    }
  }
}

void count_colisions(hash_table_t *hash_table)
{
  int colisions = 0;

  int array_numColisions[11] = {0};
  for (unsigned int i = 0; i < hash_table->hash_table_size; i++)
  {
    if (hash_table->heads[i] == NULL)
    {
      array_numColisions[0]++;
    }
    if (hash_table->heads[i] != NULL)
    {
      colisions = 1;
      hash_table_node_t *tmp = hash_table->heads[i];
      while (tmp->next != NULL)
      {
        colisions++;
        tmp = tmp->next;
      }
      array_numColisions[colisions]++;
    }
  }

  printf("Number of positions on hash table with x words: \n");
  int n_colisions = 0;
  for (int i = 0; i < 11; i++)
  {
    if (array_numColisions[i] != 0)
    {
      printf("  %i words: %i\n", i, array_numColisions[i]);
    }
    if (i > 1)
    {
      n_colisions = n_colisions + array_numColisions[i];
    }
  }
  printf("\nRate of colisions: %0.02f%%\n", (double)n_colisions / hash_table->hash_table_size * 100);
}

//------------------------------------------------------------

//
// add edges to the word ladder graph (mostly do be done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative, *next_node, *current_node;

  //
  // complete this
  //
  // find link to representative node
  for (representative = node; representative->representative != representative; representative = representative->representative)
    ;

  // path compression
  for (current_node = node; current_node != representative; current_node = next_node)
  {
    next_node = current_node->representative;
    current_node->representative = representative;
  }

  return representative;
}

static void add_edge(hash_table_t *hash_table, hash_table_node_t *from, const char *word)
{
  hash_table_node_t *to, *from_representative, *to_representative;
  adjacency_node_t *link;

  to = find_word(hash_table, word, 0);

  //
  // complete this

  if (to == NULL)
  {
    return;
  }

  if (to != NULL)
  {

    link = allocate_adjacency_node();
    link->vertex = to;
    link->next = NULL;

    if (from->head == NULL)
    {
      from->head = link;
      from->number_of_edges++;
    }
    else
    {
      link->next = from->head;
      from->head = link;
      from->number_of_edges++;
    }

    link = allocate_adjacency_node();
    link->vertex = from;
    link->next = NULL;

    if (to->head == NULL)
    {
      to->head = link;
      to->number_of_edges++;
    }
    else
    {
      link->next = to->head;
      to->head = link;
      to->number_of_edges++;
    }

    hash_table->number_of_edges++;

    from_representative = find_representative(from);
    to_representative = find_representative(to);

    if (from_representative != to_representative)
    {
      if (from_representative->number_of_vertices > to_representative->number_of_vertices)
      {
        to_representative->representative = from_representative;
        from_representative->number_of_vertices += to_representative->number_of_vertices;
      }
      else
      {
        from_representative->representative = to_representative;
        to_representative->number_of_vertices += from_representative->number_of_vertices;
      }
    }
  }
}

//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word, int *individual_characters)
{
  int byte0, byte1;

  while (*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if (byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if ((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr, "break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters, char word[_max_word_size_])
{
  int code;

  while (*individual_characters != 0)
  {
    code = *(individual_characters++);
    if (code < 0x80)
      *(word++) = (char)code;
    else if (code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr, "make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0'; // mark the end
}

static void similar_words(hash_table_t *hash_table, hash_table_node_t *from)
{
  static const int valid_characters[] =
      {                                                                                          // unicode!
       0x2D,                                                                                     // -
       0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,             // A B C D E F G H I J K L M
       0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,             // N O P Q R S T U V W X Y Z
       0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,             // a b c d e f g h i j k l m
       0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,             // n o p q r s t u v w x y z
       0xC1, 0xC2, 0xC9, 0xCD, 0xD3, 0xDA,                                                       // Á Â É Í Ó Ú
       0xE0, 0xE1, 0xE2, 0xE3, 0xE7, 0xE8, 0xE9, 0xEA, 0xED, 0xEE, 0xF3, 0xF4, 0xF5, 0xFA, 0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
       0};
  int i, j, k, individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word, individual_characters);
  for (i = 0; individual_characters[i] != 0; i++)
  {
    k = individual_characters[i];
    for (j = 0; valid_characters[j] != 0; j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters, new_word);
      // avoid duplicate cases
      if (strcmp(new_word, from->word) > 0)
        add_edge(hash_table, from, new_word);
    }
    individual_characters[i] = k;
  }
}

//
// breadth-first search (to be done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//

static hash_table_node_t **breadh_first_search(int maximum_number_of_vertices, hash_table_node_t **list_of_vertices, hash_table_node_t *origin, hash_table_node_t *goal)
{
  // complete this
  // bfs find the shortest path from origin to goal
  list_of_vertices[0] = origin;
  origin->visited = 1;
  origin->previous = NULL;
  int j = 1;
  int i = 0;

  while (i < j && i < maximum_number_of_vertices)
  {

    hash_table_node_t *current = list_of_vertices[i];
    adjacency_node_t *adjacency_node = current->head;

    while (adjacency_node != NULL)
    {
      if (adjacency_node->vertex->visited == 0)
      {
        adjacency_node->vertex->visited = 1;
        adjacency_node->vertex->previous = current;
        list_of_vertices[j] = adjacency_node->vertex;
        j++;
      }

      adjacency_node = adjacency_node->next;
    }
    i++;
  }

  origin->last_word = list_of_vertices[j - 1];

  // mark all vertices as not visited
  for (int i = 0; i < j; i++)
  {
    list_of_vertices[i]->visited = 0;
  }

  return list_of_vertices;
}

//
// list all vertices belonging to a connected component (complete this)
//

static void list_connected_component(hash_table_t *hash_table, const char *word)
{
  //
  // complete this
  //

  // find the word in the hash table
  hash_table_node_t *node = find_word(hash_table, word, 0);

  if (node == NULL)
  {
    printf("Word not found.\n");
    return;
  }

  // create a list of vertices empty
  hash_table_node_t **list_of_vertices;
  list_of_vertices = (hash_table_node_t **)malloc((size_t)node->representative->number_of_vertices * sizeof(hash_table_node_t *));
  if (list_of_vertices == NULL)
  {
    fprintf(stderr, "malloc failed: out of memory\n");
    exit(1);
  }

  for (unsigned int i = 0u; i < node->representative->number_of_vertices; i++)
    list_of_vertices[i] = NULL;

  printf("Connected component %s belgons to: \n", node->word);

  // breadth first search
  list_of_vertices = breadh_first_search(node->representative->number_of_vertices, list_of_vertices, node, NULL);

  // print the path from origin to goal or print all the vertices if goal is null
  for (int i = 0; i < node->representative->number_of_vertices; i++)
  {
    printf("[%d] %s \n", i, list_of_vertices[i]->word);
  }

  // free the list of vertices
  free(list_of_vertices);
}

//
// compute the diameter of a connected component (optional)
//

static int largest_diameter;
static hash_table_node_t *largest_diameter_node;

static int connected_component_diameter(hash_table_node_t *node)
{
  int diameter = 0;

  hash_table_node_t **list_of_vertices;
  list_of_vertices = (hash_table_node_t **)malloc((size_t)node->representative->number_of_vertices * sizeof(hash_table_node_t *));
  if (list_of_vertices == NULL)
  {
    fprintf(stderr, "malloc failed: out of memory\n");
    exit(1);
  }

  list_of_vertices = breadh_first_search(node->representative->number_of_vertices, list_of_vertices, node, NULL);

  // calculate diameter
  for (unsigned int i = 0; i < node->representative->number_of_vertices; i++)
  {
    hash_table_node_t *current = list_of_vertices[i];
    int x = 0;
    while (current != NULL)
    {
      current = current->previous;
      x++;
    }
    if (x > diameter)
    {
      diameter = x;
    }
  }

  if (diameter > largest_diameter)
  {
    largest_diameter = diameter;
    largest_diameter_node = node;
  }

  free(list_of_vertices);

  return diameter;
}

//
// find the shortest path from a given word to another given word (to be done)
//

static void path_finder(hash_table_t *hash_table, const char *from_word, const char *to_word)
{

  printf("path from %s to %s:\n", from_word, to_word);

  hash_table_node_t *from = find_word(hash_table, from_word, 0);
  hash_table_node_t *to = find_word(hash_table, to_word, 0);

  // create a list of vertices empty
  hash_table_node_t **list_of_vertices;
  list_of_vertices = (hash_table_node_t **)malloc((size_t)from->representative->number_of_vertices * sizeof(hash_table_node_t *));
  if (list_of_vertices == NULL)
  {
    fprintf(stderr, "malloc failed: out of memory\n");
    exit(1);
  }

  for (unsigned int i = 0u; i < from->representative->number_of_vertices; i++)
    list_of_vertices[i] = 0;

  // breadth first search
  list_of_vertices = breadh_first_search(from->representative->number_of_vertices, list_of_vertices, to, from);

  // print the path from origin to goal or print all the vertices if goal is null
  hash_table_node_t *current = from;
  int x = 0;
  while (current != NULL)
  {
    printf("[%d] %s \n", x, current->word);
    current = current->previous;
    x++;
  }

  // free the list of vertices
  free(list_of_vertices);
}

//
// some graph information (optional)
//

static void graph_info(hash_table_t *hash_table)
{
  int number_of_connected_components = 0;

  hash_table_node_t *node;
  unsigned int i;

  for (i = 0u; i < hash_table->hash_table_size; i++)
  {
    for (node = hash_table->heads[i]; node != NULL; node = node->next)
    {
      if (node != NULL && node->representative == node)
      {
        number_of_connected_components++;
      }
    }
  }

  printf("Number of vertices in the graph: %d\n", hash_table->number_of_entries);
  printf("Number of edges in the graph: %d\n", hash_table->number_of_edges);
  printf("Number of connected components: %d\n", number_of_connected_components);

  unsigned int j = 0;
  int diameter = 0;
  int array[number_of_connected_components];

  for (i = 0u; i < hash_table->hash_table_size; i++)
  {
    for (node = hash_table->heads[i]; node != NULL; node = node->next)
    {
      if (node != NULL && node->representative == node)
      {
        diameter = connected_component_diameter(node);
        array[j] = diameter;
        j++;
      }
    }
  }

  // sort the array
  int current = 0;
  for (i = 0; i < number_of_connected_components; i++)
  {
    for (j = i + 1; j < number_of_connected_components; j++)
    {
      if (array[i] > array[j])
      {
        current = array[i];
        array[i] = array[j];
        array[j] = current;
      }
    }
  }

  printf("\nNumber of connected components with a diameter of: \n");
  int count = 1;
  for (i = 0; i < number_of_connected_components; i++)
  {
    if (i == number_of_connected_components - 1 && array[i] != array[i - 1])
    {
      printf("  %d : %d\n", array[i], count);
      break;
    }

    if (array[i] == array[i + 1])
    {
      count++;
    }
    else
    {
      printf("  %d : %d\n", array[i], count);
      count = 1;
    }
  }

  printf("\nLargest word ladder: \n");
  path_finder(hash_table, largest_diameter_node->word, largest_diameter_node->last_word->word);
}

//
// main program
//

int main(int argc, char **argv)
{
  char word[100], from[100], to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;
  clock_t tic = clock();

  // initialize hash table
  hash_table = hash_table_create();
  // printf("hash table size: %u\n",hash_table->hash_table_size);
  //  read words
  fp = fopen((argc < 2) ? "wordlist-big-latest.txt" : argv[1], "rb");
  if (fp == NULL)
  {
    fprintf(stderr, "main: unable to open the words file\n");
    exit(1);
  }
  while (fscanf(fp, "%99s", word) == 1)
  {
    (void)find_word(hash_table, word, 1);
  }

  fclose(fp);

  // find all similar words
  for (i = 0u; i < hash_table->hash_table_size; i++)
    for (node = hash_table->heads[i]; node != NULL; node = node->next)
      similar_words(hash_table, node);

  clock_t toc = clock();

  printf("\nHash table construction time: %0.03f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

  // ask what to do
  for (;;)
  {
    fprintf(stderr, "\n");
    fprintf(stderr, "Your wish is my command:\n");
    fprintf(stderr, "  1 WORD                          (list the connected component WORD belongs to)\n");
    fprintf(stderr, "  2 FROM TO                       (list the shortest path from FROM to TO)\n");
    fprintf(stderr, "  3 See Hash Table                \n");
    fprintf(stderr, "  4 See Colisions                 \n");
    fprintf(stderr, "  5 See graph info                \n");
    fprintf(stderr, "  6 (terminate)\n");
    fprintf(stderr, "> ");
    if (scanf("%99s", word) != 1)
      break;
    command = atoi(word);
    if (command == 1)
    {
      if (scanf("%99s", word) != 1)
        break;
      list_connected_component(hash_table, word);
    }
    else if (command == 2)
    {
      if (scanf("%99s", from) != 1)
        break;
      if (scanf("%99s", to) != 1)
        break;
      path_finder(hash_table, from, to);
    }
    else if (command == 3)
    {
      print_table(hash_table);
    }
    else if (command == 4)
    {
      count_colisions(hash_table);
    }
    else if (command == 5)
    {
      graph_info(hash_table);
    }
    else if (command == 6)
    {
      break;
    }
  }

  // clean up
  hash_table_free(hash_table);
  return 0;
}