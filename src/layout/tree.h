#ifndef UI_TREE_H
#define UI_TREE_H

typedef struct panel PANEL;

/* uTox panel draw hierarchy. */
extern PANEL panel_notify_generic;

/* TODO remove this and expose it differently */
extern PANEL    panel_root,
                    panel_main,
                    panel_chat,

                    panel_overhead,

                    panel_splash_page,
                    panel_profile_password;

#endif // UI_TREE_H
