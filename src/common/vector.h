#ifndef VECTOR_H
#define VECTOR_H

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

// Struktura wektora elementów dowolnego typu.
struct vector {
  size_t num;           // Liczba elementów.
  size_t max_num;       // Maksymalna liczba elementów.
  size_t el_size;       // Rozmiar pojedynczego elementu.
  void *data;           // Wskaźnik na dane.
};

/*
 * Funkcja tworząca wektor. 
 * W przypadku błędu my_errno i errno będą ustawione i funkcja zwróci
 * wektor z zerową maksymalną liczbą elementów.
 */
struct vector vector_create(size_t element_size);

/*
 * Kopiuje wartość trzymaną pod adresem arg do wektora.
 * W przypadku błędu my_errno i errno będą ustawione i funkcja nie zmieni
 * zawartości wektora.
 */
void vector_pushback(struct vector *vec, void *arg);

/*
 * Czyści wektor, na każdym elemencie wywołuje destruktor [dest].
 * Jeśli dest == NULL, to tylko ustawia liczbę elementów na 0.
 * Nie zwalnia pamięci wektora.
 */
void vector_clear(struct vector *vec, void (*dest)(void *));

/*
 * Niszczy wektor. Wywołuje na nim vector_clear i następnie zwalnia
 * pamięć wektora.
 */
void vector_destroy(struct vector *vec, void (*dest)(void *));

// Zwraca wskaźnik na n-ty element wektora.
void *vector_get(struct vector *vec, size_t n);

// Zwraca liczbę elementów wektora.
size_t vector_size(struct vector *vec);

#endif /*VECTOR_H*/
