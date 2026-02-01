/*******************************************************************************

License: 
This software and/or related materials was developed at the National Institute
of Standards and Technology (NIST) by employees of the Federal Government
in the course of their official duties. Pursuant to title 17 Section 105
of the United States Code, this software is not subject to copyright
protection and is in the public domain. 

This software and/or related materials have been determined to be not subject
to the EAR (see Part 734.3 of the EAR for exact details) because it is
a publicly available technology and software, and is freely distributed
to any interested party with no licensing requirements.  Therefore, it is 
permissible to distribute this software as a free download from the internet.

Disclaimer: 
This software and/or related materials was developed to promote biometric
standards and biometric technology testing for the Federal Government
in accordance with the USA PATRIOT Act and the Enhanced Border Security
and Visa Entry Reform Act. Specific hardware and software products identified
in this software were used in order to perform the software development.
In no case does such identification imply recommendation or endorsement
by the National Institute of Standards and Technology, nor does it imply that
the products and equipment identified are necessarily the best available
for the purpose.

This software and/or related materials are provided "AS-IS" without warranty
of any kind including NO WARRANTY OF PERFORMANCE, MERCHANTABILITY,
NO WARRANTY OF NON-INFRINGEMENT OF ANY 3RD PARTY INTELLECTUAL PROPERTY
or FITNESS FOR A PARTICULAR PURPOSE or for any purpose whatsoever, for the
licensed product, however used. In no event shall NIST be liable for any
damages and/or costs, including but not limited to incidental or consequential
damages of any kind, including economic damage or injury to property and lost
profits, regardless of whether NIST shall be advised, have reason to know,
or in fact shall know of the possibility.

By using this software, you agree to bear all risk relating to quality,
use and performance of the software and/or related materials.  You agree
to hold the Government harmless from any claim arising from your use
of the software.

*******************************************************************************/


/***********************************************************************
      LIBRARY: LFS - NIST Latent Fingerprint System

      FILE:    IMGUTIL.C
      AUTHOR:  Michael D. Garris
      DATE:    03/16/1999
      UPDATED: 03/16/2005 by MDG

      Contains general support image routines required by the NIST
      Latent Fingerprint System (LFS).

***********************************************************************
               ROUTINES:
                        bits_6to8()
                        bits_8to6()
                        gray2bin()
                        pad_uchar_image()
                        fill_holes()
                        free_path()
                        search_in_direction()

***********************************************************************/

#include <stdio.h>
#include <memory.h>
#include <lfs.h>

/*************************************************************************
**************************************************************************
#cat: bits_6to8 - Takes an array of unsigned characters and bitwise shifts
#cat:             each value 2 postitions to the left.  This is equivalent
#cat:             to multiplying each value by 4.  This puts original values
#cat:             on the range [0..64) now on the range [0..256).  Another
#cat:             way to say this, is the original 6-bit values now fit in
#cat:             8 bits.  This is to be used to undo the effects of bits_8to6.

   Input:
      idata - input array of unsigned characters
      iw    - width (in characters) of the input array
      ih    - height (in characters) of the input array
   Output:
      idata - contains the bit-shifted results
**************************************************************************/
void bits_6to8(unsigned char *idata, const int iw, const int ih)
{
   int i, isize;
   unsigned char *iptr;

   isize = iw * ih;
   iptr = idata;
   for(i = 0; i < isize; i++){
      /* Multiply every pixel value by 4 so that [0..64) -> [0..255) */
      *iptr++ <<= 2;
   }
}

/*************************************************************************
**************************************************************************
#cat: bits_8to6 - Takes an array of unsigned characters and bitwise shifts
#cat:             each value 2 postitions to the right.  This is equivalent
#cat:             to dividing each value by 4.  This puts original values
#cat:             on the range [0..256) now on the range [0..64).  Another
#cat:             way to say this, is the original 8-bit values now fit in
#cat:             6 bits.  I would really like to make this dependency
#cat:             go away.

   Input:
      idata - input array of unsigned characters
      iw    - width (in characters) of the input array
      ih    - height (in characters) of the input array
   Output:
      idata - contains the bit-shifted results
**************************************************************************/
void bits_8to6(unsigned char *idata, const int iw, const int ih)
{
   int i, isize;
   unsigned char *iptr;

   isize = iw * ih;
   iptr = idata;
   for(i = 0; i < isize; i++){
      /* Divide every pixel value by 4 so that [0..256) -> [0..64) */
      *iptr++ >>= 2;
   }
}

/*************************************************************************
**************************************************************************
#cat: gray2bin - Takes an 8-bit threshold value and two 8-bit pixel values.
#cat:            Those pixels in the image less than the threhsold are set
#cat:            to the first specified pixel value, whereas those pixels
#cat:            greater than or equal to the threshold are set to the second
#cat:            specified pixel value.  On application for this routine is
#cat:            to convert binary images from 8-bit pixels valued {0,255} to
#cat:            {1,0} and vice versa.

   Input:
      thresh      - 8-bit pixel threshold
      less_pix    - pixel value used when image pixel is < threshold
      greater_pix - pixel value used when image pixel is >= threshold
      bdata       - 8-bit image data
      iw          - width (in pixels) of the image
      ih          - height (in pixels) of the image
   Output:
      bdata       - altered 8-bit image data
**************************************************************************/
void gray2bin(const int thresh, const int less_pix, const int greater_pix,
              unsigned char *bdata, const int iw, const int ih)
{
   int i;

   for(i = 0; i < iw*ih; i++){
      if(bdata[i] >= thresh)
         bdata[i] = (unsigned char)greater_pix;
      else
         bdata[i] = (unsigned char)less_pix;
   }
}

/*************************************************************************
**************************************************************************
#cat: pad_uchar_image - Copies an 8-bit grayscale images into a larger
#cat:                   output image centering the input image so as to
#cat:                   add a specified amount of pixel padding along the
#cat:                   entire perimeter of the input image.  The amount of
#cat:                   pixel padding and the intensity of the pixel padding
#cat:                   are specified.  An alternative to padding with a
#cat:                   constant intensity would be to copy the edge pixels
#cat:                   of the centered image into the adjacent pad area.

   Input:
      idata     - input 8-bit grayscale image
      iw        - width (in pixels) of the input image
      ih        - height (in pixels) of the input image
      pad       - size of padding (in pixels) to be added
      pad_value - intensity of the padded area
   Output:
      optr      - points to the newly padded image
      ow        - width (in pixels) of the padded image
      oh        - height (in pixels) of the padded image
   Return Code:
      Zero     - successful completion
      Negative - system error
**************************************************************************/
int pad_uchar_image(unsigned char **optr, int *ow, int *oh,
                    unsigned char *idata, const int iw, const int ih,
                    const int pad, const int pad_value)
{
   unsigned char *pdata, *pptr, *iptr;
   int i, pw, ph;
   int pad2, psize;

   /* Account for pad on both sides of image */
   pad2 = pad<<1;

   /* Compute new pad sizes */
   pw = iw + pad2;
   ph = ih + pad2;
   psize = pw * ph;

   /* Allocate padded image */
   pdata = (unsigned char *)malloc(psize * sizeof(unsigned char));
   if(pdata == (unsigned char *)NULL){
      fprintf(stderr, "ERROR : pad_uchar_image : malloc : pdata\n");
      return(-160);
   }

   /* Initialize values to a constant PAD value */
   memset(pdata, pad_value, psize);

   /* Copy input image into padded image one scanline at a time */
   iptr = idata;
   pptr = pdata + (pad * pw) + pad;
   for(i = 0; i < ih; i++){
      memcpy(pptr, iptr, iw);
      iptr += iw;
      pptr += pw;
   }

   *optr = pdata;
   *ow = pw;
   *oh = ph;
   return(0);
}

/*************************************************************************
**************************************************************************
#cat: fill_holes - Takes an input image and analyzes triplets of horizontal
#cat:              pixels first and then triplets of vertical pixels, filling
#cat:              in holes of width 1.  A hole is defined as the case where
#cat:              the neighboring 2 pixels are equal, AND the center pixel
#cat:              is different.  Each hole is filled with the value of its
#cat:              immediate neighbors. This routine modifies the input image.

   Input:
      bdata - binary image data to be processed
      iw    - width (in pixels) of the binary input image
      ih    - height (in pixels) of the binary input image
   Output:
      bdata - points to the results
**************************************************************************/
#include <omp.h>

void fill_holes(unsigned char *bdata, const int iw, const int ih)
{
    int ix, iy;
    unsigned char *lptr, *mptr, *rptr;
    unsigned char *tptr, *bptr;
    unsigned char *sptr;

#pragma omp parallel for private(ix, lptr, mptr, rptr, sptr)
    for(iy = 0; iy < ih; iy++){
        sptr = bdata + iy * iw + 1;

        lptr = sptr - 1;
        mptr = sptr;
        rptr = sptr + 1;

        for(ix = 1; ix < iw - 1; ix++){
            if((*lptr != *mptr) && (*lptr == *rptr)){
                *mptr = *lptr;
                lptr += 2;
                mptr += 2;
                rptr += 2;
                ix++;
            } else {
                lptr++;
                mptr++;
                rptr++;
            }
        }
    }

#pragma omp parallel for private(iy, tptr, mptr, bptr, sptr)
    for(ix = 0; ix < iw; ix++){
        sptr = bdata + iw + ix;

        tptr = sptr - iw;
        mptr = sptr;
        bptr = sptr + iw;

        for(iy = 1; iy < ih - 1; iy++){
            if((*tptr != *mptr) && (*tptr == *bptr)){
                *mptr = *tptr;
                tptr += 2 * iw;
                mptr += 2 * iw;
                bptr += 2 * iw;
                iy++;
            } else {
                tptr += iw;
                mptr += iw;
                bptr += iw;
            }
        }
    }
}

/*************************************************************************
**************************************************************************
#cat: free_path - Traverses a straight line between 2 pixel points in an
#cat:             image and determines if a "free path" exists between the
#cat:             2 points by counting the number of pixel value transitions
#cat:             between adjacent pixels along the trajectory.

   Input:
      x1       - x-pixel coord of first point
      y1       - y-pixel coord of first point
      x2       - x-pixel coord of second point
      y2       - y-pixel coord of second point
      bdata    - binary image data (0==while & 1==black)
      iw       - width (in pixels) of image
      ih       - height (in pixels) of image
      lfsparms - parameters and threshold for controlling LFS
   Return Code:
      TRUE      - free path determined to exist
      FALSE     - free path determined not to exist
      Negative  - system error
**************************************************************************/
int free_path(const int x1, const int y1, const int x2, const int y2,
              unsigned char *bdata, const int iw, const int ih,
              const LFSPARMS *lfsparms)
{
   int *x_list, *y_list, num;
   int ret;
   int i, trans, preval, nextval;

   /* Compute points along line segment between the two points. */
   if((ret = line_points(&x_list, &y_list, &num, x1, y1, x2, y2)))
      return(ret);

   /* Intialize the number of transitions to 0. */
   trans = 0;
   /* Get the pixel value of first point along line segment. */
   preval = *(bdata+(y1*iw)+x1);

   /* Foreach remaining point along line segment ... */
   for(i = 1; i < num; i++){
      /* Get pixel value of next point along line segment. */
      nextval = *(bdata+(y_list[i]*iw)+x_list[i]);

      /* If next pixel value different from previous pixel value ... */
      if(nextval != preval){
         /* Then we have detected a transition, so bump counter. */
         trans++;
         /* If number of transitions seen > than threshold (ex. 2) ... */
         if(trans > lfsparms->maxtrans){
            /* Deallocate the line segment's coordinate lists. */
            free(x_list);
            free(y_list);
            /* Return free path to be FALSE. */
            return(FALSE);
         }
         /* Otherwise, maximum number of transitions not yet exceeded. */
         /* Assign the next pixel value to the previous pixel value.   */
         preval = nextval;
      }
      /* Otherwise, no transition detected this interation. */

   }

   /* If we get here we did not exceed the maximum allowable number        */
   /* of transitions.  So, deallocate the line segment's coordinate lists. */
   free(x_list);
   free(y_list);

   /* Return free path to be TRUE. */
   return(TRUE);
}

/*************************************************************************
**************************************************************************
#cat: search_in_direction - Takes a specified maximum number of steps in a
#cat:                specified direction looking for the first occurence of
#cat:                a pixel with specified value.  (Once found, adjustments
#cat:                are potentially made to make sure the resulting pixel
#cat:                and its associated edge pixel are 4-connected.)

   Input:
      pix       - value of pixel to be searched for
      strt_x    - x-pixel coord to start search
      strt_y    - y-pixel coord to start search
      delta_x   - increment in x for each step
      delta_y   - increment in y for each step
      maxsteps  - maximum number of steps to conduct search
      bdata     - binary image data (0==while & 1==black)
      iw        - width (in pixels) of image
      ih        - height (in pixels) of image
   Output:
      ox        - x coord of located pixel
      oy        - y coord of located pixel
      oex       - x coord of associated edge pixel
      oey       - y coord of associated edge pixel
   Return Code:
      TRUE      - pixel of specified value found
      FALSE     - pixel of specified value NOT found
**************************************************************************/
int search_in_direction(int *ox, int *oy, int *oex, int *oey, const int pix,
                const int strt_x, const int strt_y,
                const double delta_x, const double delta_y, const int maxsteps,
                unsigned char *bdata, const int iw, const int ih)
{

   int i, x, y, px, py;
   double fx, fy;

   /* Set previous point to starting point. */
   px = strt_x;
   py = strt_y;
   /* Set floating point accumulators to starting point. */
   fx = (double)strt_x;
   fy = (double)strt_y;

   /* Foreach step up to the specified maximum ... */
   for(i = 0; i < maxsteps; i++){

      /* Increment accumulators. */
      fx += delta_x;
      fy += delta_y;
      /* Round to get next step. */
      x = sround(fx);
      y = sround(fy);

      /* If we stepped outside the image boundaries ... */
      if((x < 0) || (x >= iw) ||
         (y < 0) || (y >= ih)){
         /* Return FALSE (we did not find what we were looking for). */
         *ox = -1;
         *oy = -1;
         *oex = -1;
         *oey = -1;
         return(FALSE);
      }

      /* Otherwise, test to see if we found our pixel with value 'pix'. */
      if(*(bdata+(y*iw)+x) == pix){
         /* The previous and current pixels form a feature, edge pixel */
         /* pair, which we would like to use for edge following.  The  */
         /* previous pixel may be a diagonal neighbor however to the   */
         /* current pixel, in which case the pair could not be used by */
         /* the contour tracing (which requires the edge pixel in the  */
         /* pair neighbor to the N,S,E or W.                           */
         /* This routine adjusts the pair so that the results may be   */
         /* used by the contour tracing.                               */
         fix_edge_pixel_pair(&x, &y, &px, &py, bdata, iw, ih);

         /* Return TRUE (we found what we were looking for). */
         *ox = x;
         *oy = y;
         *oex = px;
         *oey = py;
         return(TRUE);
      }

      /* Otherwise, still haven't found pixel with desired value, */
      /* so set current point to previous and take another step.  */
      px = x;
      py = y;
   }

   /* Return FALSE (we did not find what we were looking for). */
   *ox = -1;
   *oy = -1;
   *oex = -1;
   *oey = -1;
   return(FALSE);
}

