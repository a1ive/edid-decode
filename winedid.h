#ifndef WINEDID_H
#define WINEDID_H

#ifdef __cplusplus
extern "C" {
#endif

	int get_monitor_edid(int index, void* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // WINEDID_H