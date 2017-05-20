/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_JDriver,
    0x3447af0f,0xab2b,0x404a,0x88,0x45,0x74,0x9f,0xc4,0xd4,0x31,0x34);
// {3447af0f-ab2b-404a-8845-749fc4d43134}
