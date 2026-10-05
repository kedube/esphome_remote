#pragma once

// Which favorite lists the remote uses, and keeping the ones Home Assistant
// publishes (see Favorites from Home Assistant in the README).
//
// The remote uses the list it saved from Home Assistant, or local_entities.h
// until it has one. A list Home Assistant changes is saved as the remote goes
// to sleep and used from the next wake: the trackers and Home Assistant's
// subscriptions keep pointers into the lists in use, so those never change
// while the remote is awake. Instead the remote restarts to use new lists
// straight away when they are the first from Home Assistant, or when nobody
// has pressed a button since it woke (it isn't in use).

#include <cstddef>
#include <cstdint>
#include <string>

#include "favorites_lists.h"

// The Home Assistant entity, and its attribute, that holds the lists. Set
// FAVORITES_ENTITY to "" in local_entities.h to use local_entities.h only.
#ifndef FAVORITES_ENTITY
#define FAVORITES_ENTITY "sensor.remote_favorites"
#endif

#ifndef FAVORITES_ATTRIBUTE
#define FAVORITES_ATTRIBUTE "lists"
#endif

inline bool favorites_from_home_assistant_enabled() { return FAVORITES_ENTITY[0] != '\0'; }

// The lists in use. Loaded on the first call.
const FavoriteSet &active_favorites();

// Whether they came from Home Assistant (otherwise local_entities.h).
bool active_favorites_from_home_assistant();

// Why a list would not fit in the remote's memory, or "" when it would.
// candidate is the new list, current the one in use.
using FavoritesFitCheck = std::string (*)(const FavoriteSet &candidate, const FavoriteSet &current);

// Home Assistant sent the lists' text. may_restart: nobody has pressed a
// button since the remote woke, so it may restart to use changed lists now.
void favorites_received(const char *text, size_t len, FavoritesFitCheck fits, bool may_restart);

// Saves a changed list. Called as the remote goes to sleep or restarts.
void favorites_save_pending();

// New lists from Home Assistant are saved: restart to use them.
bool favorites_restart_requested();

// The lists in use and what became of the last ones Home Assistant sent, for
// the Favorites page in Info.
struct FavoritesSummary {
  bool from_home_assistant = false;
  int lists = 0;  // lists with favorites
  int favorites = 0;
  FavoritesNote note = FAVORITES_NOTE_NONE;
  int note_line = 0;  // for FAVORITES_NOTE_NOT_USED, the line at fault; 0 for none
};
FavoritesSummary favorites_summary();

// What the remote is using and what became of the last list Home Assistant
// sent, for the Favorites status sensor. The listener hears it now and on
// every change.
void favorites_set_status_listener(void (*listener)(const std::string &status));
