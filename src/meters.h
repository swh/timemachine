#ifndef METERS_H
#define METERS_H

#define lin2db(lin) (20.0f * log10(lin))
#define db2lin(db)  (pow(10, db / 20.0f))

void bind_meters(void);

void update_meters(float amp[]);

#endif
