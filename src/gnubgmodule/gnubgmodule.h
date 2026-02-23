/*
 * gnubgmodule.h
 *
 * Originally by Joseph Heled <joseph@gnubg.org>, 2000
 * Adapted for Python 3 and Meson build system by David Reay
 * <dr323090@falmouth.ac.uk>
 *
 * Copyright 2000 Joseph Heled
 * Copyright 2025 David Reay
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SRC_GNUBGMODULE_GNUBGMODULE_H_
#define SRC_GNUBGMODULE_GNUBGMODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Set package data directory (e.g. .../gnubg/data) so weights/bearoff are found. Call before gnubg_lib_init_for_python. */
void gnubg_lib_set_pkg_datadir(const char *path);

/* Ensure neural nets and match equity are loaded (for standalone Python module use). */
void gnubg_lib_init_for_python(void);

#ifdef __cplusplus
}
#endif

#endif  // SRC_GNUBGMODULE_GNUBGMODULE_H_
