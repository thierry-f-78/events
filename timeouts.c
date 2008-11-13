/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

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

// insert
ev_errors ev_timeout_add(struct ev_timeout_node *n, struct timeval *tv,
                         ev_timeout_run func, void *arg,
                         struct ev_timeout_node **node) {
	//                      n; // pour la recherche
	struct ev_timeout_node *c; // nouveau noeud
	struct ev_timeout_node *d; // deuxieme moitié d'un noeud splitté
	unsigned long long int date;
	unsigned char idx;

	date   = (unsigned int)tv->tv_sec;
	date <<= 32;
	date  |= (unsigned int)tv->tv_usec;

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
		mask2 = (n->date & n->mask) ^ (date & n->mask);

		// on passe au noeud suivant
		if ( mask2 == 0x0ULL ) {

			// choix du noeud suivant 
			// on choisis si on va vers le haut ou le bas
			idx = ( date & mask2nextbit(n->mask) ) != 0x0ULL;

			if (n->go[idx] == NULL) // on est arrivé au bout
				break;

			n = n->go[idx];
		}

		// sinon il faut couper ce noeud en deux
		else {
			unsigned int bits;
			unsigned long long int mask1;
	
			d = (struct ev_timeout_node *)malloc(sizeof(struct ev_timeout_node));
			if (d == NULL)	
				return EV_ERR_MALLOC;

			// gcc -O2 est plus rapide lorsque cette operation est repetée ...
			// curieux ... ???? ...
			mask2 = (n->date & n->mask) ^ (date & n->mask);
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
			mask1   = ( ~ ( ( 1ULL << (bits+1) ) - 1ULL ) ) & n->mask;
			mask2   =     ( ( 1ULL << (bits+1) ) - 1ULL )   & n->mask;

			// index du nouveau noeud
			idx = ( date & mask2nextbit(mask1) ) != 0x0ULL;

			// split
			d->date          = n->date;
			d->mask          = mask2;
			d->parent        = n;
			d->func          = n->func;
			d->arg           = n->arg;

			d->go[0]         = n->go[0];
			if (d->go[0] != NULL)
				d->go[0]->parent = d;

			d->go[1]         = n->go[1];
			if (d->go[1] != NULL)
				d->go[1]->parent = d;

			// orig
			n->mask     = mask1;
			n->go[!idx] = d;

			// fin
			break;
		}
	}


	// nouveau noeud
	c = (struct ev_timeout_node *)malloc(sizeof(struct ev_timeout_node));
	if (c == NULL)	
		return EV_ERR_MALLOC;

	c->date    = date;
	// mask          0 0 1 1 0 0 0 0
	// mask2nextbit  0 0 0 0 1 0 0 0
	// << 1          0 0 0 1 0 0 0 0
	// -1            0 0 0 0 1 1 1 1
	c->mask    = ( mask2nextbit(n->mask) << 1) - 1;
	c->go[0]   = NULL;
	c->go[1]   = NULL;
	c->parent  = n;
	c->func    = func;
	c->arg     = arg;
	n->go[idx] = c;


	if (node != NULL)
		*node = c;

	return EV_OK;
}

void ev_timeout_del(struct ev_timeout_node *val) {
	struct ev_timeout_node *n; // parent of deleted node
	struct ev_timeout_node *p; // prent of parent node
	struct ev_timeout_node *b; // broser of deleted node

	/* initial status:     after deletion status:
	 *
    *       [p]               [p]
    *      0   1             0   1
	 *           \                 \    
	 *           [n]               [b]
	 *          0   1
	 *         /     \        
	 *       [b]    [val]
	 */

	// get parent
	n = val->parent;

	// get parent of parent
	p = n->parent;

	// si p est NULL, c'est que l'on est au bout
	if (p == NULL) {

		// get broser
		if (n->go[0] == val)
			n->go[0] = NULL;
		else
			n->go[1] = NULL;
	}

	// 
	else {

		// get broser
		if (n->go[0] == val)
			b = n->go[1];
		else
			b = n->go[0];

		// extend broser mask
		b->mask |= n->mask;

		// change links
		b->parent = p;
		if (p->go[0] == n)
			p->go[0] = b;
		else
			p->go[1] = b;

		free(n);
	}

	free(val);
}

/* get time */
struct ev_timeout_node *ev_timeout_exists(struct ev_timeout_node *base,
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
			return base;
	
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
struct ev_timeout_node *ev_timeout_get_min(struct ev_timeout_node *base) {
	if (base->go[0] != NULL)
		base = base->go[0];
	else if (base->go[1] != NULL)
		base = base->go[1];
	else
		return NULL;
		
	while (base->go[0] != NULL)
		base = base->go[0];
	return base;
}

/* get min time */
struct ev_timeout_node *ev_timeout_get_max(struct ev_timeout_node *base) {
	if (base->go[1] != NULL)
		base = base->go[1];
	else if (base->go[0] != NULL)
		base = base->go[0];
	else
		return NULL;
		
	while (base->go[1] != NULL)
		base = base->go[1];
	return base;
}

/* get next node */
struct ev_timeout_node *ev_timeout_get_next(struct ev_timeout_node *current) {
	struct ev_timeout_node *p;
	int sens = 1;

	// on verifie si on peut aller a gauche,
	//   -> on ne peut pas si a gauche c'est NULL, ou si on en viens
	// sinon on regarde si on peut aller a droite
	//   -> on ne peut pas si a droite c'est NULL ou si on e viens
	// sinon on recule
	//
	p = current;
	while (p != NULL) {

		// on peut aller a gauche, si 
		//  - on avance
		//  - c'est pas NULL
		//  - on en viens pas
		if (sens == 1 && p->go[0] != NULL && p->go[0] != current)
			p = p->go[0];

		// on peut aller a droite si
		//  - c'est pas NULL
		//  - on en viens pas
		else if (p->go[1] != NULL && p->go[1] != current) {
			sens = 1;
			p = p->go[1];
		}

		// si on ne peut plus avancer, c'est que on a trouve
		// le noeud suivant
		else if (p->go[1] == NULL && p->go[0] == NULL && p != current)
			return p;

		// si on peut reculer, on recule
		else if (p->parent != NULL) {
			sens = 0;
			current = p;
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
	struct ev_timeout_node *p;
	int sens = 1;

	// on verifie si on peut aller a gauche,
	//   -> on ne peut pas si a gauche c'est NULL, ou si on en viens
	// sinon on regarde si on peut aller a droite
	//   -> on ne peut pas si a droite c'est NULL ou si on e viens
	// sinon on recule
	//
	p = current;
	while (p != NULL) {

		// on peut aller a droite si
		//  - on avance
		//  - c'est pas NULL
		//  - on en viens pas
		if (sens == 1 && p->go[1] != NULL && p->go[1] != current)
			p = p->go[1];

		// on peut aller a gauche, si 
		//  - c'est pas NULL
		//  - on en viens pas
		else if (p->go[0] != NULL && p->go[0] != current) {
			sens = 1;
			p = p->go[0];
		}

		// si on ne peut plus avancer, c'est que on a trouve
		// le noeud suivant
		else if (p->go[1] == NULL && p->go[0] == NULL && p != current)
			return p;

		// si on peut reculer, on recule
		else if (p->parent != NULL) {
			sens = 0;
			current = p;
			p = p->parent;
		}

		// si on ne peut pas, c'est qu'il n'y a plus de valeurs
		else
			return NULL;
	}

	return NULL;
}
