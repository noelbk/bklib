/* vercmp.h - compare version strings like debian's dpkg
 *
 * Noel Burton-Krahn <noel@burton-krahn.com>
 * Feb 16, 2005
 *
 */

#ifndef VERCMP_H_INCLUDED
#define VERCMP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int
vercmp(char *v1, char *v2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VERCMP_H_INCLUDED */
