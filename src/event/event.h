#ifndef SIBA_EVENT_h
#define SIBA_EVENT_h

void            event_register();
void            (*event_grep(int code))();
size_t          event_exec(SB_ACTION);
size_t          event_add(int code, SB_ACTION);

#endif