#ifndef CUSBFX2_H_INCLUDED
#define CUSBFX2_H_INCLUDED

struct cusbfx2_handle;
typedef struct cusbfx2_handle cusbfx2_handle;

struct cusbfx2_transfer;
typedef struct cusbfx2_transfer cusbfx2_transfer;

typedef void (*cusbfx2_transfer_cb_fn)(gpointer buf, gint length);


gint
cusbfx2_init(void);
void
cusbfx2_exit(void);
cusbfx2_handle *
cusbfx2_open(guint8 id, guint8 *firmware, const gchar *firmware_id);
void
cusbfx2_close(cusbfx2_handle *h);
gint
cusbfx2_bulk_transfer(cusbfx2_handle *h, guint8 endpoint, guint8 *data, gint length);
cusbfx2_transfer *
cusbfx2_init_bulk_transfer(cusbfx2_handle *h, guint8 endpoint, gint length, gint nqueues,
						   cusbfx2_transfer_cb_fn callback);
void
cusbfx2_start_transfer(cusbfx2_transfer *transfer);
void
cusbfx2_free_transfer(cusbfx2_transfer *transfer);
int
cusbfx2_poll(void);

#endif	/* CUSBFX2_H_INCLUDED */
