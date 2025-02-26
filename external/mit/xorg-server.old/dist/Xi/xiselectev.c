/*
 * Copyright 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif


#include "dixstruct.h"
#include "windowstr.h"
#include "exglobals.h"
#include "exevents.h"
#include <X11/extensions/XI2proto.h>

#include "xiselectev.h"

/**
 * Check the given mask (in len bytes) for invalid mask bits.
 * Invalid mask bits are any bits above XI2LastEvent.
 *
 * @return BadValue if at least one invalid bit is set or Success otherwise.
 */
int XICheckInvalidMaskBits(ClientPtr client, unsigned char *mask, int len)
{
    if (len >= XIMaskLen(XI2LASTEVENT))
    {
        int i;
        for (i = XI2LASTEVENT + 1; i < len * 8; i++)
        {
            if (BitIsOn(mask, i))
            {
                client->errorValue = i;
                return BadValue;
            }
        }
    }

    return Success;
}

int
SProcXISelectEvents(ClientPtr client)
{
    char n;
    int i;
    int len;
    xXIEventMask* evmask;

    REQUEST(xXISelectEventsReq);
    swaps(&stuff->length, n);
    REQUEST_AT_LEAST_SIZE(xXISelectEventsReq);
    swapl(&stuff->win, n);
    swaps(&stuff->num_masks, n);

    len = stuff->length - bytes_to_int32(sizeof(xXISelectEventsReq));
    evmask = (xXIEventMask*)&stuff[1];
    for (i = 0; i < stuff->num_masks; i++)
    {
        if (len < bytes_to_int32(sizeof(xXIEventMask)))
            return BadLength;
        len -= bytes_to_int32(sizeof(xXIEventMask));
        swaps(&evmask->deviceid, n);
        swaps(&evmask->mask_len, n);
        if (len < evmask->mask_len)
            return BadLength;
        len -= evmask->mask_len;
        evmask = (xXIEventMask*)(((char*)&evmask[1]) + evmask->mask_len * 4);
    }

    return (ProcXISelectEvents(client));
}

int
ProcXISelectEvents(ClientPtr client)
{
    int rc, num_masks;
    WindowPtr win;
    DeviceIntPtr dev;
    DeviceIntRec dummy;
    xXIEventMask *evmask;
    int *types = NULL;
    int len;

    REQUEST(xXISelectEventsReq);
    REQUEST_AT_LEAST_SIZE(xXISelectEventsReq);

    if (stuff->num_masks == 0)
        return BadValue;

    rc = dixLookupWindow(&win, stuff->win, client, DixReceiveAccess);
    if (rc != Success)
        return rc;

    len = sz_xXISelectEventsReq;

    /* check request validity */
    evmask = (xXIEventMask*)&stuff[1];
    num_masks = stuff->num_masks;
    while(num_masks--)
    {
        len += sizeof(xXIEventMask) + evmask->mask_len * 4;

        if (bytes_to_int32(len) > stuff->length)
            return BadLength;

        if (evmask->deviceid != XIAllDevices &&
            evmask->deviceid != XIAllMasterDevices)
            rc = dixLookupDevice(&dev, evmask->deviceid, client, DixUseAccess);
        else {
            /* XXX: XACE here? */
        }
        if (rc != Success)
            return rc;

        /* hierarchy event mask is not allowed on devices */
        if (evmask->deviceid != XIAllDevices && evmask->mask_len >= 1)
        {
            unsigned char *bits = (unsigned char*)&evmask[1];
            if (BitIsOn(bits, XI_HierarchyChanged))
            {
                client->errorValue = XI_HierarchyChanged;
                return BadValue;
            }
        }

        /* Raw events may only be selected on root windows */
        if (win->parent && evmask->mask_len >= 1)
        {
            unsigned char *bits = (unsigned char*)&evmask[1];
            if (BitIsOn(bits, XI_RawKeyPress) ||
                BitIsOn(bits, XI_RawKeyRelease) ||
                BitIsOn(bits, XI_RawButtonPress) ||
                BitIsOn(bits, XI_RawButtonRelease) ||
                BitIsOn(bits, XI_RawMotion))
            {
                client->errorValue = XI_RawKeyPress;
                return BadValue;
            }
        }

        if (XICheckInvalidMaskBits(client, (unsigned char*)&evmask[1],
                                   evmask->mask_len * 4) != Success)
            return BadValue;

        evmask = (xXIEventMask*)(((unsigned char*)evmask) + evmask->mask_len * 4);
        evmask++;
    }

    if (bytes_to_int32(len) != stuff->length)
        return BadLength;

    /* Set masks on window */
    evmask = (xXIEventMask*)&stuff[1];
    num_masks = stuff->num_masks;
    while(num_masks--)
    {
        if (evmask->deviceid == XIAllDevices ||
            evmask->deviceid == XIAllMasterDevices)
        {
            dummy.id = evmask->deviceid;
            dev = &dummy;
        } else
            dixLookupDevice(&dev, evmask->deviceid, client, DixUseAccess);
        if (XISetEventMask(dev, win, client, evmask->mask_len * 4,
                           (unsigned char*)&evmask[1]) != Success)
            return BadAlloc;
        evmask = (xXIEventMask*)(((unsigned char*)evmask) + evmask->mask_len * 4);
        evmask++;
    }

    RecalculateDeliverableEvents(win);

    free(types);
    return Success;
}


int
SProcXIGetSelectedEvents(ClientPtr client)
{
    char n;

    REQUEST(xXIGetSelectedEventsReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXIGetSelectedEventsReq);
    swapl(&stuff->win, n);

    return (ProcXIGetSelectedEvents(client));
}

int
ProcXIGetSelectedEvents(ClientPtr client)
{
    int rc, i;
    WindowPtr win;
    char n;
    char *buffer = NULL;
    xXIGetSelectedEventsReply reply;
    OtherInputMasks *masks;
    InputClientsPtr others = NULL;
    xXIEventMask *evmask = NULL;
    DeviceIntPtr dev;
    uint32_t length;

    REQUEST(xXIGetSelectedEventsReq);
    REQUEST_SIZE_MATCH(xXIGetSelectedEventsReq);

    rc = dixLookupWindow(&win, stuff->win, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    reply.repType = X_Reply;
    reply.RepType = X_XIGetSelectedEvents;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    reply.num_masks = 0;

    masks = wOtherInputMasks(win);
    if (masks)
    {
	for (others = wOtherInputMasks(win)->inputClients; others;
	     others = others->next) {
	    if (SameClient(others, client)) {
                break;
            }
        }
    }

    if (!others)
    {
        WriteReplyToClient(client, sizeof(xXIGetSelectedEventsReply), &reply);
        return Success;
    }

    buffer = calloc(MAXDEVICES, sizeof(xXIEventMask) + pad_to_int32(XI2MASKSIZE));
    if (!buffer)
        return BadAlloc;

    evmask = (xXIEventMask*)buffer;
    for (i = 0; i < MAXDEVICES; i++)
    {
        int j;
        unsigned char *devmask = others->xi2mask[i];

        if (i > 2)
        {
            rc = dixLookupDevice(&dev, i, client, DixGetAttrAccess);
            if (rc != Success)
                continue;
        }


        for (j = XI2MASKSIZE - 1; j >= 0; j--)
        {
            if (devmask[j] != 0)
            {
                int mask_len = (j + 4)/4; /* j is an index, hence + 4, not + 3 */
                evmask->deviceid = i;
                evmask->mask_len = mask_len;
                reply.num_masks++;
                reply.length += sizeof(xXIEventMask)/4 + evmask->mask_len;

                if (client->swapped)
                {
                    swaps(&evmask->deviceid, n);
                    swaps(&evmask->mask_len, n);
                }

                memcpy(&evmask[1], devmask, j + 1);
                evmask = (xXIEventMask*)((char*)evmask +
                           sizeof(xXIEventMask) + mask_len * 4);
                break;
            }
        }
    }

    /* save the value before SRepXIGetSelectedEvents swaps it */
    length = reply.length;
    WriteReplyToClient(client, sizeof(xXIGetSelectedEventsReply), &reply);

    if (reply.num_masks)
        WriteToClient(client, length * 4, buffer);

    free(buffer);
    return Success;
}

void SRepXIGetSelectedEvents(ClientPtr client,
                            int len, xXIGetSelectedEventsReply *rep)
{
    char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->num_masks, n);
    WriteToClient(client, len, (char *)rep);
}


