/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

var pbA=IO.digitalIn(IO.pin9, IO.pullDown);
var pbB=IO.digitalIn(IO.pin10, IO.pullDown);

AJ.onAttach = function()
{
    print('attached');
    pbA.setTrigger(IO.fallingEdge, function(){ AJ.notification(AJ.notification.Warning, "Button A pressed").send(200); })
    pbB.setTrigger(IO.fallingEdge, function(){ AJ.notification(AJ.notification.Emergency, "Button B pressed").send(200); })
}

AJ.onDetach = function()
{
    pbA.setTrigger(IO.disable);
    pbB.setTrigger(IO.disable);
}
