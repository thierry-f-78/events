/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>

#include "events.h"

static inline int bitscan(register unsigned long long int v) {
	register int b = 0;

	// methode trouvée dans haproxy
	if (v & 0xffffffff00000000ULL) { v &= 0xffffffff00000000ULL; b += 32; }
	if (v & 0xffff0000ffff0000ULL) { v &= 0xffff0000ffff0000ULL; b += 16; }
	if (v & 0xff00ff00ff00ff00ULL) { v &= 0xff00ff00ff00ff00ULL; b +=  8; }
	if (v & 0xf0f0f0f0f0f0f0f0ULL) { v &= 0xf0f0f0f0f0f0f0f0ULL; b +=  4; }
	if (v & 0xccccccccccccccccULL) { v &= 0xccccccccccccccccULL; b +=  2; }
	if (v & 0xaaaaaaaaaaaaaaaaULL) { v &= 0xaaaaaaaaaaaaaaaaULL; b +=  1; }
	return b;
}

static inline unsigned long long int
                           mask2nextbit(register unsigned long long int val) {
	// si le masque est vide, le prochain bit est le premier
	if (val == 0x0ULL)
		return 0x8000000000000000ULL;
	
	// val         0 0 0 1 1 1 0 0
	// val >> 1    0 0 0 0 1 1 1 0
	// ^ val       0 0 0 1 0 0 1 0
	// & val >> 1  0 0 0 0 0 0 1 0
	return ( ( val >> 1 ) ^ val ) & ( val >> 1 );
}

// nouveau noeud
struct ev_timeout_node *ev_timeout_new(void) {
	return (struct ev_timeout_node *)calloc(1, sizeof(struct ev_timeout_node));
}

// insert
ev_errors ev_timeout_insert(struct ev_timeout_basic_node *b,
                         struct ev_timeout_node *t) {
	struct ev_timeout_basic_node *n; // new node
	struct ev_timeout_basic_node *l; // new leaf
	struct ev_timeout_basic_node *base;
	struct ev_timeout_node *c;
	unsigned char idx;

	// node dispatch
	base = b;
	n    = &t->node;
	l    = &t->leaf;

	// find insert point
	while (1) {
		unsigned long long int mask2;

		// date1  0 0 1 0 1 1 0 1 0 1
		// date2  1 1 0 0 1 0 0 1 1 0
		// mask   0 0 0 1 1 1 1 0 0 0
		//
		// 1 ^ 2  1 1 1 0 0 1 0 0 1 1
		// & mask 0 0 0 0 0 1 0 0 0 0

		// si les deux date sont egales apres l'application du masque
		// mask2 sera == 0, sinon sont premier bit à 1 sera celui de
		// la cission du neoud
		mask2 = (b->date & b->mask) ^ (l->date & b->mask);

		// on passe au noeud suivant
		if ( mask2 == 0x0ULL ) {

			// choix du noeud suivant 
			// on choisis si on va vers le haut ou le bas
			idx = ( l->date & mask2nextbit(b->mask) ) != 0x0ULL;

			// on est au bout
			if (b->go[idx] == NULL) {

				// on est au bout, on ne fait qu'un ajout
				if (b == base)
					break;

				// c'est un doublon
				else {
					c = b->me;
					t->prev = c->prev;
					t->next = c;
					c->prev = t;
					t->prev->next = t;
					return EV_OK;
				}
			}

			// noeud suivant
			b = b->go[idx];
		}

		// sinon il faut couper ce noeud en deux
		else {
			unsigned int bits;
			unsigned long long int mask1;

			// gcc -O2 est plus rapide lorsque cette operation est repetée ...
			// curieux ... ???? ...
			mask2 = (b->date & b->mask) ^ (l->date & b->mask);
			bits = bitscan(mask2);
	
			// ce que l'on veut
			// n° bit  7 6 5 4 3 2 1 0
			//                 v
			// mask1   0 0 1 1 0 0 0 0
			// mask2   0 0 0 0 1 1 1 0
		
			// comment on fait
			//
			// mask2 = 1                0 0 0 0 0 0 0 1
			// mask2 << bits+1          0 0 0 1 0 0 0 0
			// mask2 - 1                0 0 0 0 1 1 1 1
			//
			// mask1 ~= mask2 & n->mask 0 0 1 1 0 0 0 0
			// mask2 &= n->mask         0 0 0 0 1 1 1 0
		
			// plus rapide qu'en mutualisant les operations
			mask1   = ( ~ ( ( 1ULL << (bits+1) ) - 1ULL ) ) & b->mask;
			mask2   =     ( ( 1ULL << (bits+1) ) - 1ULL )   & b->mask;

			// index du nouveau noeud
			idx = ( l->date & mask2nextbit(mask1) ) != 0x0ULL;

			// split
			n->date     = b->date;
			n->mask     = mask1;
			n->parent   = b->parent;
			n->go[!idx] = b;
			if (n->parent->go[0] == b)
				n->parent->go[0] = n;
			else
				n->parent->go[1] = n;

			// orig
			b->mask     = mask2;
			b->parent   = n;
			b           = n;

			// fin
			break;
		}
	}

	// mask          0 0 1 1 0 0 0 0
	// mask2nextbit  0 0 0 0 1 0 0 0
	// << 1          0 0 0 1 0 0 0 0
	// -1            0 0 0 0 1 1 1 1
	l->mask     = (mask2nextbit(b->mask) << 1) - 1ULL;
	l->go[0]    = NULL;
	l->go[1]    = NULL;
	l->parent   = b;
	b->go[idx]  = l;

	return EV_OK;
}

void ev_timeout_free(struct ev_timeout_node *val) {
	free(val);
}

void ev_timeout_remove(struct ev_timeout_node *val) {
	struct ev_timeout_basic_node *m; // my leaf
	struct ev_timeout_basic_node *n; // parent of deleted leaf
	struct ev_timeout_basic_node *p; // parent of parent leaf
	struct ev_timeout_basic_node *b; // brother of deleted leaf
	struct ev_timeout_basic_node *d; // deleted node

	/* initial status:     after deletion status:
	 *
	 *       [p]               [p]
	 *      0   1             0   1
	 *           \                 \    
	 *           [n]               [b]
	 *          0   1
	 *         /     \        
	 *       [b]     [m]
	 */

	// cas d'une liste chainée ou l'on n'est pas le noeud de debut
	if (val->leaf.parent == NULL) {
		val->next->prev = val->prev;
		val->prev->next = val->next;
		// set for no swapping
		n = &val->node;
	}

	// cas d'une liste chainée ou l'on est le noeud de debut
	else if (val->next != val && val->leaf.parent != NULL) {
		// copy node values
		val->next->leaf.date   = val->leaf.date;
		val->next->leaf.mask   = val->leaf.mask;
		// relink next node
		val->next->leaf.parent = val->leaf.parent;
		if (val->leaf.parent->go[0] == &val->leaf)
			val->leaf.parent->go[0] = &val->next->leaf;
		else
			val->leaf.parent->go[1] = &val->next->leaf;
		// unlink current node
		val->next->prev            = val->prev;
		val->prev->next            = val->next;
		// set this for swapping node
		n = &val->next->node;
	}

	// sinon, o supprime une feuille
	else {

		// me
		m = &val->leaf;
	
		// get parent
		n = m->parent;
	
		// get parent of parent
		p = n->parent;
	
		// si p est NULL, c'est que l'on est au bout
		if (p == NULL) {
	
			// get brother
			if (n->go[0] == m)
				n->go[0] = NULL;
			else
				n->go[1] = NULL;
		}
	
		// sinon on recupere le noeud frangin
		else {
	
			// get brother
			if (n->go[0] == m)
				b = n->go[1];
			else
				b = n->go[0];
	
			// extend brother mask
			b->mask |= n->mask;
	
			// change links
			b->parent = p;
			if (p->go[0] == n)
				p->go[0] = b;
			else
				p->go[1] = b;
		}
	}

	// swap unused node (n) with deleted node (val->node)
	if (n != &val->node) {
		d                = &val->node;
		n->date          = d->date;
		n->mask          = d->mask;
		n->parent        = d->parent;
		n->go[0]         = d->go[0];
		n->go[1]         = d->go[1];
		if (n->go[0] != NULL)
			n->go[0]->parent = n;
		if (n->go[1] != NULL)
			n->go[1]->parent = n;
		if (n->parent != NULL) {
			if (n->parent->go[0] == d)
				n->parent->go[0] = n;
			else
				n->parent->go[1] = n;
		}
	}
}

/* get time */
struct ev_timeout_node *ev_timeout_exists(struct ev_timeout_basic_node *base,
                                          struct timeval *tv) {
	unsigned long long int date;
	unsigned char idx;

	date   = (unsigned int)tv->tv_sec;
	date <<= 32;
	date  |= (unsigned int)tv->tv_usec;

	while (1) {

		// si le masque le noeud ne correspond pas, la valeur n'existe pas
		if ( ( (date & base->mask) ^ (base->date & base->mask) ) != 0x0ULL )
			return NULL;

		// si la date correspond et  que l'ion ne peux plus avancer, on a fini
		if ( ( date ^ base->date ) == 0x0ULL && base->go[0] == NULL && base->go[1] == NULL )
			return base->me;
	
		// on cherche le prochain noeud
		idx = ( date & mask2nextbit(base->mask) ) != 0x0ULL;

		// si il n'y en a pas, c'est que la valeur n'existe pas
		if ( base->go[idx] == NULL )
			return NULL;

		// suivanrt ...
		base = base->go[idx];
	}

}

/* get max time */
struct ev_timeout_node *ev_timeout_get_min(struct ev_timeout_basic_node *base) {
	if (base->go[0] != NULL)
		base = base->go[0];
	else if (base->go[1] != NULL)
		base = base->go[1];
	else
		return NULL;
		
	while (base->go[0] != NULL)
		base = base->go[0];
	return base->me;
}

/* get min time */
struct ev_timeout_node *ev_timeout_get_max(struct ev_timeout_basic_node *base) {
	if (base->go[1] != NULL)
		base = base->go[1];
	else if (base->go[0] != NULL)
		base = base->go[0];
	else
		return NULL;
		
	while (base->go[1] != NULL)
		base = base->go[1];
	return base->me;
}

/* get next node */
struct ev_timeout_node *ev_timeout_get_next(struct ev_timeout_node *current) {
	struct ev_timeout_basic_node *m;
	struct ev_timeout_basic_node *p;
	int sens = 1;

	// on verifie si on peut aller a gauche,
	//   -> on ne peut pas si a gauche c'est NULL, ou si on en viens
	// sinon on regarde si on peut aller a droite
	//   -> on ne peut pas si a droite c'est NULL ou si on e viens
	// sinon on recule
	//
	p = &current->leaf;
	m = p;
	while (p != NULL) {

		// on peut aller a gauche, si 
		//  - on avance
		//  - c'est pas NULL
		//  - on en viens pas
		if (sens == 1 && p->go[0] != NULL && p->go[0] != m)
			p = p->go[0];

		// on peut aller a droite si
		//  - c'est pas NULL
		//  - on en viens pas
		else if (p->go[1] != NULL && p->go[1] != m) {
			sens = 1;
			p = p->go[1];
		}

		// si on ne peut plus avancer, c'est que on a trouve
		// le noeud suivant
		else if (p->go[1] == NULL && p->go[0] == NULL && p != m)
			return p->me;

		// si on peut reculer, on recule
		else if (p->parent != NULL) {
			sens = 0;
			m = p;
			p = p->parent;
		}

		// si on ne peut pas, c'est qu'il n'y a plus de valeurs
		else
			return NULL;
	}

	return NULL;
}

/* get prev node */
struct ev_timeout_node *ev_timeout_get_prev(struct ev_timeout_node *current) {
	struct ev_timeout_basic_node *m;
	struct ev_timeout_basic_node *p;
	int sens = 1;

	// on verifie si on peut aller a gauche,
	//   -> on ne peut pas si a gauche c'est NULL, ou si on en viens
	// sinon on regarde si on peut aller a droite
	//   -> on ne peut pas si a droite c'est NULL ou si on e viens
	// sinon on recule
	//
	p = &current->leaf;
	m = p;
	while (p != NULL) {

		// on peut aller a droite si
		//  - on avance
		//  - c'est pas NULL
		//  - on en viens pas
		if (sens == 1 && p->go[1] != NULL && p->go[1] != m)
			p = p->go[1];

		// on peut aller a gauche, si 
		//  - c'est pas NULL
		//  - on en viens pas
		else if (p->go[0] != NULL && p->go[0] != m) {
			sens = 1;
			p = p->go[0];
		}

		// si on ne peut plus avancer, c'est que on a trouve
		// le noeud suivant
		else if (p->go[1] == NULL && p->go[0] == NULL && p != m)
			return p->me;

		// si on peut reculer, on recule
		else if (p->parent != NULL) {
			sens = 0;
			m = p;
			p = p->parent;
		}

		// si on ne peut pas, c'est qu'il n'y a plus de valeurs
		else
			return NULL;
	}

	return NULL;
}
