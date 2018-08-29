#include "osu.h"

#include <string.h>

#define RNG_ROUNDS 50
#define RNG_BOUNDARY 0.5

/**
 * Parses a raw beatmap line into a beatmap_meta struct pointed to by *meta.
 * Returns the number of tokens read.
 */
static int parse_beatmap_line(char *line, struct beatmap_meta *meta);

/**
 * Parses a key:value set into *meta.
 */
static void parse_beatmap_token(char *key, char *value,
	struct beatmap_meta *meta);

/**
 * Parses a raw hitobject line into a hitpoint struct pointed to by *point.
 * Returns the number of tokens read.
 */
static int parse_hitobject_line(char *line, int columns,
	struct hitpoint *point);

/**
 * Populates *start and *end with data from hitpoint *point.
 */
static void hitpoint_to_action(char *keys, struct hitpoint *point,
	struct action *start, struct action *end);

// const char col_keys[] = { 'a', 's', 'd', 'f', ' ', 'j', 'k', 'l', ';' };
// const char col_keys_odd[] = "asdf jkl[";
const char col_keys[] = "asdfjkl[";

int find_beatmap(char *base, char *partial, char **map)
{
	if (!base || !partial || !map) {
		debug("received null pointer");
		return 0;
	}

	int folder_len = 0;
	char *folder = NULL;

	if (!(folder_len = find_partial_file(base, partial, &folder))) {
		debug("couldn't find folder (%s)", partial);
		return 0;
	}

	int map_len = 256, base_len = strlen(base);
	*map = malloc(map_len);

	// Absolute path to our base.
	strcpy(*map, base);
	// A.p. to the beatmap folder.
	strcpy(*map + base_len, folder);
	// Add a trailing seperator and terminating zero.
	strcpy(*map + base_len + folder_len, (char[2]){(char)SEPERATOR, '\0'});

	free(folder);

	int beatmap_len = 0;
	char *beatmap = NULL;

	if (!(beatmap_len = find_partial_file(*map, partial, &beatmap))) {
		debug("couldn't find beatmap in %s", *map);
		return 0;
	}

	// This is now the absolute path to our beatmap.
	strcpy(*map + base_len + folder_len + 1, beatmap);

	free(beatmap);

	map_len = base_len + folder_len + 1 + beatmap_len;

	// Change block size to fit what we really need.
	*map = realloc(*map, map_len + 1);

	// Verify that the file we found is beatmap.
	if (strcmp(*map + map_len - 4, ".osu") != 0) {
		debug("%s is not a beatmap", *map);
		
		free(*map);

		return 0;
	}

	return map_len;
}

// TODO: Inefficient as it calls realloc() for every parsed line. Allocate
// 	 memory in chunks and copy it to adequqtely sized buffer once done.
int parse_beatmap(char *file, struct hitpoint **points,
	struct beatmap_meta **meta)
{
	if (!points || !meta || !file) {
		debug("received null pointer");
		return 0;
	}

	FILE *stream;

	if (!(stream = fopen(file, "r"))) {
		debug("couldn't open file %s", file);
		return 0;
	}

	*points = NULL;
	*meta = calloc(1, sizeof(struct beatmap_meta));

	const int line_len = 256;
	char *line = malloc(line_len);

	struct hitpoint cur_point;
	int num_parsed = 0, cur_section = 0;
	size_t hp_size = sizeof(struct hitpoint);

	while (fgets(line, line_len, stream)) {
		switch (cur_section) {
		// [Metadata]
		case 3:
			parse_beatmap_line(line, *meta);
			break;
		case 4: parse_beatmap_line(line, *meta);
			break;
		// [HitObjects]
		case 7:
			parse_hitobject_line(line, meta[0]->columns, &cur_point);

			*points = realloc(*points, ++num_parsed * hp_size);
			points[0][num_parsed - 1] = cur_point;
			break;
		}

		if (line[0] == '[' && line[strlen(line) - 3] == ']')
			cur_section++;
	}

	free(line);
	fclose(stream);

	return num_parsed;
}

// TODO: This function is not thread safe.
static int parse_beatmap_line(char *line, struct beatmap_meta *meta)
{
	int i = 0;
	// strtok() modfies its arguments, work with a copy.
	char *ln = strdup(line);
	char *token = NULL, *key = NULL, *value = NULL;

	// Metadata lines come in key:value pairs.
	token = strtok(ln, ":");
	while (token != NULL) {
		switch (i++) {
		case 0: key = strdup(token);
			break;
		case 1: value = strdup(token);

			parse_beatmap_token(key, value, meta);

			break;
		}

		token = strtok(NULL, ":");
	}

	free(ln);
	free(key);
	free(token);
	free(value);

	return i;
}

static void parse_beatmap_token(char *key, char *value,
	struct beatmap_meta *meta)
{
	if (!key || !value || !meta) {
		debug("received null pointer");
		return;
	}

	// Always ignore last two characters since .osu files are CRLF by
	// default.
	if (!(strcmp(key, "Title"))) {
		value[strlen(value) - 2] = '\0';

		strcpy(meta->title, value);
	} else if (!(strcmp(key, "Artist"))) {
		value[strlen(value) - 2] = '\0';

		strcpy(meta->artist, value);
	} else if (!(strcmp(key, "Version"))) {
		value[strlen(value) - 2] = '\0';

		strcpy(meta->version, value);
	} else if (!(strcmp(key, "BeatmapID"))) {
		meta->map_id = atoi(value);
	} else if (!(strcmp(key, "BeatmapSetID"))) {
		meta->set_id = atoi(value);
	} else if (!(strcmp(key, "CircleSize"))) {
		meta->columns = atoi(value);
	}
}

// TODO: This function is not thread safe.
static int parse_hitobject_line(char *line, int columns, struct hitpoint *point)
{
	int end_time = 0, secval = 0, i = 0;
	char *ln = strdup(line), *token = NULL;

	// Line is expected to follow the following format:
	// x, y, time, type, hitSound, extras (= a:b:c:d:)
	token = strtok(ln, ",");
	while (token != NULL) {
		secval = strtol(token, NULL, 10);

		switch (i++) {
		// X
		case 0: point->column = secval / (COLS_WIDTH / columns);
			break;
		// Start time
		case 2: point->start_time = secval;
			break;
		// Extra string, first element is either 0 or end time
		case 5:
			end_time = strtol(strtok(token, ":"), NULL, 10);

			// If end is 0 this is not a hold note, just tap.
			point->end_time = end_time ? end_time :
				point->start_time + TAPTIME_MS;

			break;
		}

		token = strtok(NULL, ",");
	}

	free(ln);
	free(token);

	return i;
}

int parse_hitpoints(int count, int columns, struct hitpoint **points,
	struct action **actions)
{
	// Allocate enough memory for all actions at once.
	*actions = malloc((2 * count) * sizeof(struct action));

	int num_actions = 0, i = 0;
	struct hitpoint *cur_point;

	char *key_subset = malloc(columns + 1);
	key_subset[columns] = '\0';

	const int col_size = sizeof(col_keys) - 1;
	const int subset_offset = (col_size / 2) - (columns / 2);

	memmove(key_subset, col_keys + subset_offset,
		col_size - (subset_offset * 2));

	if (columns % 2) {
		memmove(key_subset + columns / 2 + 1, key_subset + columns / 2,
			columns / 2);
		key_subset[columns / 2] = ' ';
	}

	while (i < count) {
		cur_point = (*points) + i++;

		// Don't care about the order here.
		struct action *end = *actions + num_actions++;
		struct action *start = *actions + num_actions++;

		hitpoint_to_action(key_subset, cur_point, start, end);
	}

	free(key_subset);

	// TODO: Check if all *actions memory was used and free() if applicable.

	return num_actions;
}

static void hitpoint_to_action(char *keys, struct hitpoint *point,
	struct action *start, struct action *end)
{
	end->time = point->end_time;
	start->time = point->start_time;

	end->down = 0;		// Keyup.
	start->down = 1;	// Keydown.

	char key = keys[point->column];

	end->key = key;
	start->key = key;
}

// TODO: Implement a more efficient sorting algorithm than selection sort.
int sort_actions(int total, struct action **actions)
{
	int min, i, j;
	struct action *act = *actions, tmp;

	// For every element but the last, which will end up to be the biggest.
	for (i = 0; i < (total - 1); i++) {
		min = i;

		// In the subarray starting at a[j]...
		for (j = i; j < total; j++)
			// ...find the smallest element.
			if ((act + j)->time < (act + min)->time) min = j;

		// Swap current element with the smallest element of subarray.
		tmp = act[i];
		act[i] = act[min];
		act[min] = tmp;
	}

	return i - total + 1;
}

// TODO: This function is retarded, fix it and add actual humanization.
void humanize_hitpoints(int total, struct hitpoint **points, int level)
{
	if (!level) {
		return;
	}

	int i, offset;
	struct hitpoint *p = NULL;
	for (i = 0; i < total; i++) {
		p = *points + i;

		// [0, level]
		offset = generate_number(level, RNG_ROUNDS, RNG_BOUNDARY);
		
		// [-(level / 2), (level / 2)]
		offset -= (level / 2);

		p->end_time += offset;
		p->start_time += offset;
	}
}
