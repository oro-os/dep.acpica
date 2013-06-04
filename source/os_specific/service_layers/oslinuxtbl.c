/******************************************************************************
 *
 * Module Name: oslinuxtbl - Linux OSL for obtaining ACPI tables
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2013, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include "acpi.h"
#include "acmacros.h"
#include "actables.h"
#include "platform/acenv.h"
#include "acpidump.h"

#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("oslinuxtbl")


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef PATH_MAX
#define PATH_MAX 256
#endif


/* Local prototypes */

static ACPI_STATUS
OslReadTableFromFile (
    FILE                    *TableFile,
    ACPI_SIZE               FileOffset,
    char                    *Signature,
    ACPI_TABLE_HEADER       **Table);

static ACPI_STATUS
OslMapTable (
    ACPI_SIZE               Address,
    char                    *Signature,
    ACPI_TABLE_HEADER       **Table);

static ACPI_STATUS
OslGetOverrideTable (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address);

static ACPI_STATUS
OslGetDynamicSsdt (
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address);

static ACPI_STATUS
OslAddTablesToList (
    char                    *Directory);


/* File locations */

#define DYNAMIC_SSDT_DIR    "/sys/firmware/acpi/tables/dynamic"
#define OVERRIDE_TABLE_DIR  "/sys/firmware/acpi/tables"
#define SYSTEM_MEMORY       "/dev/mem"

/* Should we get dynamically loaded SSDTs from DYNAMIC_SSDT_DIR? */

UINT8                   Gbl_DumpDynamicSsdts = TRUE;

/* Initialization flags */

UINT8                   Gbl_TableListInitialized = FALSE;
UINT8                   Gbl_RsdpObtained = FALSE;

/* Local copies of main ACPI tables */

ACPI_TABLE_RSDP         Gbl_Rsdp;

/* List of information about obtained ACPI tables */

typedef struct          table_info
{
    struct table_info       *Next;
    UINT32                  Instance;
    char                    Signature[4];

} OSL_TABLE_INFO;

OSL_TABLE_INFO          *Gbl_TableListHead = NULL;


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByAddress
 *
 * PARAMETERS:  Address         - Physical address of the ACPI table
 *              Table           - Where a pointer to the table is returned
 *
 * RETURN:      Status; Table buffer is returned if AE_OK.
 *              AE_NOT_FOUND: A valid table was not found at the address
 *
 * DESCRIPTION: Get an ACPI table via a physical memory address.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByAddress (
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_TABLE_HEADER       **Table)
{
    ACPI_TABLE_HEADER       *MappedTable;
    ACPI_TABLE_HEADER       *LocalTable;
    ACPI_STATUS             Status;


    /* Validate the input physical address */

    if (Address < ACPI_HI_RSDP_WINDOW_BASE)
    {
        fprintf (stderr, "Invalid table address: 0x%8.8X%8.8X\n",
            ACPI_FORMAT_UINT64 (Address));
        return (AE_BAD_ADDRESS);
    }

    /* Map the table and validate it */

    Status = OslMapTable (Address, NULL, &MappedTable);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Copy table to local buffer and return it */

    LocalTable = calloc (1, MappedTable->Length);
    if (!LocalTable)
    {
        return (AE_NO_MEMORY);
    }

    ACPI_MEMCPY (LocalTable, MappedTable, MappedTable->Length);
    AcpiOsUnmapMemory (MappedTable, MappedTable->Length);

    *Table = LocalTable;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByName
 *
 * PARAMETERS:  Signature       - ACPI Signature for desired table. Must be
 *                                a null terminated 4-character string.
 *              Instance        - For SSDTs (0...n) - Must be 0 for non-SSDT
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *              AE_LIMIT: Instance is beyond valid limit
 *              AE_NOT_FOUND: A table with the signature was not found
 *
 * NOTE:        Assumes the input signature is uppercase.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByName (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    ACPI_TABLE_HEADER       *LocalTable = NULL;
    ACPI_TABLE_HEADER       *MappedTable;
    ACPI_TABLE_FADT         *Fadt;
    UINT8                   *TableData;
    UINT8                   *RsdpAddress;
    ACPI_PHYSICAL_ADDRESS   RsdpBase;
    ACPI_SIZE               RsdpSize;
    UINT8                   NumberOfTables;
    UINT8                   Revision;
    UINT8                   ItemSize;
    UINT32                  CurrentInstance = 0;
    ACPI_PHYSICAL_ADDRESS   TableAddress = 0;
    ACPI_STATUS             Status;
    UINT32                  i;


    /* Instance is only valid for SSDTs */

    if (Instance && !ACPI_COMPARE_NAME (Signature, ACPI_SIG_SSDT))
    {
        return (AE_LIMIT);
    }

    /* Get RSDP from memory on first invocation of this function */

    if (!Gbl_RsdpObtained)
    {
        RsdpBase = ACPI_HI_RSDP_WINDOW_BASE;
        RsdpSize = ACPI_HI_RSDP_WINDOW_SIZE;

        RsdpAddress = AcpiOsMapMemory (RsdpBase, RsdpSize);
        if (!RsdpAddress)
        {
            goto DumpOverrideTable;
        }

        /* Search low memory for the RSDP */

        MappedTable = ACPI_CAST_PTR (ACPI_TABLE_HEADER,
            AcpiTbScanMemoryForRsdp (RsdpAddress, RsdpSize));
        if (!MappedTable)
        {
            AcpiOsUnmapMemory (RsdpAddress, RsdpSize);
            goto DumpOverrideTable;
        }

        ACPI_MEMCPY (&Gbl_Rsdp, MappedTable, sizeof (ACPI_TABLE_RSDP));
        Gbl_RsdpObtained = TRUE;
        AcpiOsUnmapMemory (RsdpAddress, RsdpSize);
    }

    /* Requests for RSDT/XSDT are special cases */

    if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_XSDT))
    {
        if ((Gbl_Rsdp.Revision <= 1) ||
            (!Gbl_Rsdp.XsdtPhysicalAddress))
        {
            return (AE_NOT_FOUND);
        }
    }
    else if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_RSDT))
    {
        if (!Gbl_Rsdp.RsdtPhysicalAddress)
        {
            return (AE_NOT_FOUND);
        }
    }

    /* Map RSDT or XSDT based on RSDP version */

    if (Gbl_Rsdp.Revision)
    {
        Status = OslMapTable (Gbl_Rsdp.XsdtPhysicalAddress,
            ACPI_SIG_XSDT, &MappedTable);
        Revision = 2;
        ItemSize = sizeof (UINT64);
    }
    else /* No XSDT, use RSDT */
    {
        Status = OslMapTable (Gbl_Rsdp.RsdtPhysicalAddress,
            ACPI_SIG_RSDT, &MappedTable);
        Revision = 0;
        ItemSize = sizeof (UINT32);
    }

    if (ACPI_FAILURE (Status))
    {
        goto DumpOverrideTable;
    }

    /* Copy RSDT/XSDT to local buffer */

    LocalTable = calloc (1, MappedTable->Length);
    if (!LocalTable)
    {
        AcpiOsUnmapMemory (MappedTable, MappedTable->Length);
        goto DumpOverrideTable;
    }

    ACPI_MEMCPY (LocalTable, MappedTable, MappedTable->Length);
    AcpiOsUnmapMemory (MappedTable, MappedTable->Length);

    /* If RSDT/XSDT requested, we are done */

    if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_RSDT) ||
        ACPI_COMPARE_NAME (Signature, ACPI_SIG_XSDT))
    {
        if (Revision)
        {
            *Address = Gbl_Rsdp.XsdtPhysicalAddress;
        }
        else
        {
            *Address = Gbl_Rsdp.RsdtPhysicalAddress;
        }

        *Table = LocalTable;
        return (AE_OK);
    }

    TableData = ACPI_CAST8 (LocalTable) + sizeof (ACPI_TABLE_HEADER);

    /* DSDT and FACS address must be extracted from the FADT */

    if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_DSDT) ||
        ACPI_COMPARE_NAME (Signature, ACPI_SIG_FACS))
    {
        /* Get the FADT */

        if (Revision)
        {
            TableAddress = (ACPI_PHYSICAL_ADDRESS) (*ACPI_CAST64 (TableData));
        }
        else
        {
            TableAddress = (ACPI_PHYSICAL_ADDRESS) (*ACPI_CAST32 (TableData));
        }

        if (!TableAddress)
        {
            fprintf(stderr, "No FADT in memory!\n");
            goto DumpOverrideTable;
        }

        Status = OslMapTable (TableAddress, NULL,
            ACPI_CAST_PTR (ACPI_TABLE_HEADER*, &Fadt));
        if (ACPI_FAILURE (Status))
        {
            goto DumpOverrideTable;
        }

        if (!Fadt)
        {
            fprintf(stderr, "No FADT in memory!\n");
            goto DumpOverrideTable;
        }

        /* Get the appropriate address, either 32-bit or 64-bit */

        TableAddress = 0;
        if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_DSDT))
        {
            if ((Fadt->Header.Length >= 148) && Fadt->XDsdt)
            {
                TableAddress = (ACPI_PHYSICAL_ADDRESS) Fadt->XDsdt;
            }
            else if ((Fadt->Header.Length >= 44) && Fadt->Dsdt)
            {
                TableAddress = (ACPI_PHYSICAL_ADDRESS) Fadt->Dsdt;
            }
        }
        else
        {
            if ((Fadt->Header.Length >= 140) && Fadt->XFacs)
            {
                TableAddress = (ACPI_PHYSICAL_ADDRESS) Fadt->XFacs;
            }
            else if ((Fadt->Header.Length >= 40) && Fadt->Facs)
            {
                TableAddress = (ACPI_PHYSICAL_ADDRESS) Fadt->Facs;
            }
        }

        AcpiOsUnmapMemory (Fadt, Fadt->Header.Length);
        if (!TableAddress)
        {
            fprintf (stderr, "No %s in FADT!\n", Signature);
            goto DumpOverrideTable;
        }

        /* Now we can finally get the requested table (DSDT or FACS) */

        Status = OslMapTable (TableAddress, Signature, &MappedTable);
        if (ACPI_FAILURE (Status))
        {
            goto DumpOverrideTable;
        }
    }
    else
    {
        /* Case for a normal ACPI table */

        NumberOfTables =
            (LocalTable->Length - sizeof (ACPI_TABLE_HEADER)) / ItemSize;

        /* Search RSDT/XSDT for the requested table */

        for (i = 0; i < NumberOfTables; ++i, TableData += ItemSize)
        {
            if (Revision)
            {
                TableAddress =
                    (ACPI_PHYSICAL_ADDRESS) (*ACPI_CAST64 (TableData));
            }
            else
            {
                TableAddress =
                    (ACPI_PHYSICAL_ADDRESS) (*ACPI_CAST32 (TableData));
            }

            if (!TableAddress)
            {
                continue;
            }

            Status = OslMapTable (TableAddress, NULL, &MappedTable);
            if (ACPI_FAILURE (Status))
            {
                goto DumpOverrideTable;
            }

            if (!MappedTable)
            {
                continue;
            }

            /* Does this table the match the requested signature? */

            if (!ACPI_COMPARE_NAME (MappedTable->Signature, Signature))
            {
                AcpiOsUnmapMemory (MappedTable, MappedTable->Length);
                MappedTable = NULL;
                continue;
            }

            /* Match table instance (for SSDTs) */

            if (CurrentInstance != Instance)
            {
                AcpiOsUnmapMemory (MappedTable, MappedTable->Length);
                MappedTable = NULL;
                CurrentInstance++;
                continue;
            }

            break;
        }
    }

    if (CurrentInstance != Instance)
    {
        if (Gbl_DumpDynamicSsdts)
        {
            goto DumpDynamicSsdt;
        }

        goto DumpOverrideTable;
    }

    if (!MappedTable)
    {
        goto DumpOverrideTable;
    }

    /* Copy table to local buffer and return it */

    LocalTable = calloc (1, MappedTable->Length);
    if (!LocalTable)
    {
        return (AE_NO_MEMORY);
    }

    ACPI_MEMCPY (LocalTable, MappedTable, MappedTable->Length);
    AcpiOsUnmapMemory (MappedTable, MappedTable->Length);
    *Address = TableAddress;

    *Table = LocalTable;
    return (AE_OK);


DumpOverrideTable:

    /* Dump overridden table from file system instead of memory */

    Status = OslGetOverrideTable (Signature, Instance, Table, Address);

    if ((Status == AE_LIMIT) && Gbl_DumpDynamicSsdts)
    {
        goto DumpDynamicSsdt;
    }

    return (Status);


DumpDynamicSsdt:

    return (OslGetDynamicSsdt (Instance, Table, Address));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTableByIndex
 *
 * PARAMETERS:  Index           - Which table to get
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer and physical address returned if AE_OK.
 *              AE_LIMIT: Index is beyond valid limit
 *
 * DESCRIPTION: Get an ACPI table via an index value (0 through n). Returns
 *              AE_LIMIT when an invalid index is reached. Index is not
 *              necessarily an index into the RSDT/XSDT.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetTableByIndex (
    UINT32                  Index,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    OSL_TABLE_INFO          *Info;
    ACPI_STATUS             Status;
    UINT32                  i;


    /* Initialize the table list on first invocation */

    if (!Gbl_TableListInitialized)
    {
        Gbl_TableListHead = calloc (1, sizeof (OSL_TABLE_INFO));

        /* List head records the length of the list */

        Gbl_TableListHead->Instance = 0;

        /* Add all tables found in the override directory */

        Status = OslAddTablesToList (OVERRIDE_TABLE_DIR);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Add all dynamically loaded SSDTs in the dynamic directory */

        OslAddTablesToList (DYNAMIC_SSDT_DIR);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        Gbl_TableListInitialized = TRUE;
    }

    /* Validate Index */

    if (Index >= Gbl_TableListHead->Instance)
    {
        return (AE_LIMIT);
    }

    /* Point to the table list entry specified by the Index argument */

    Info = Gbl_TableListHead;
    for (i = 0; i <= Index; i++)
    {
        Info = Info->Next;
    }

    /* Now we can just get the table via the signature */

    Status = AcpiOsGetTableByName (Info->Signature, Info->Instance,
        Table, Address);
    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  Where               - Physical address of memory to be mapped
 *              Length              - How much memory to map
 *
 * RETURN:      Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION: Map physical memory into local address space.
 *
 *****************************************************************************/

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
    UINT8                   *MappedMemory;
    ACPI_PHYSICAL_ADDRESS   Offset;
    ACPI_SIZE               PageSize;
    int                     fd;


    fd = open (SYSTEM_MEMORY, O_RDONLY | O_BINARY);
    if (fd < 0)
    {
        fprintf (stderr, "Cannot open %s\n", SYSTEM_MEMORY);
        return (NULL);
    }

    /* Align the offset to use mmap */

    PageSize = sysconf (_SC_PAGESIZE);
    Offset = Where % PageSize;

    /* Map the table header to get the length of the full table */

    MappedMemory = mmap (NULL, (Length + Offset), PROT_READ, MAP_PRIVATE,
        fd, (Where - Offset));
    close (fd);

    if (MappedMemory == MAP_FAILED)
    {
        return (NULL);
    }

    return (ACPI_CAST8 (MappedMemory + Offset));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  Where               - Logical address of memory to be unmapped
 *              Length              - How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping. Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory (
    void                    *Where,
    ACPI_SIZE               Length)
{
    ACPI_PHYSICAL_ADDRESS   Offset;
    ACPI_SIZE               PageSize;


    PageSize = sysconf (_SC_PAGESIZE);
    Offset = (ACPI_PHYSICAL_ADDRESS) Where % PageSize;
    munmap ((UINT8 *) Where - Offset, (Length + Offset));
}


/******************************************************************************
 *
 * FUNCTION:    OslAddTablesToList
 *
 * PARAMETERS:  Directory           - Directory that contains the tables
 *
 * RETURN:      Status; Table list is initiated if AE_OK.
 *
 * DESCRIPTION: Add ACPI tables to the table list from a directory.
 *
 *****************************************************************************/

static ACPI_STATUS
OslAddTablesToList(
    char                    *Directory)
{
    struct stat             FileInfo;
    OSL_TABLE_INFO          *NewInfo;
    OSL_TABLE_INFO          *Info;
    struct dirent           *DirInfo;
    DIR                     *TableDir;
    char                    TempName[4];
    UINT32                  i;


    /* Open the requested directory */

    if (stat (Directory, &FileInfo) == -1)
    {
        return (AE_NOT_FOUND);
    }

    if (!(TableDir = opendir (Directory)))
    {
        return (AE_ERROR);
    }

    /* Move pointer to the end of the list */

    if (!Gbl_TableListHead)
    {
        return (AE_ERROR);
    }

    Info = Gbl_TableListHead;
    for (i = 0; i < Gbl_TableListHead->Instance; i++)
    {
        Info = Info->Next;
    }

    /* Examine all entries in this directory */

    while ((DirInfo = readdir (TableDir)) != 0)
    {
        /* Ignore meaningless files */

        if (DirInfo->d_name[0] == '.')
        {
            continue;
        }

        /* Skip any subdirectories and create a new info node */

        if (strlen (DirInfo->d_name) < 6)
        {
            NewInfo = calloc (1, sizeof (OSL_TABLE_INFO));
            if (strlen (DirInfo->d_name) > 4)
            {
                sscanf (DirInfo->d_name, "%[^1-9]%d",
                    TempName, &NewInfo->Instance);
            }
 
            /* Add new info node to global table list */

            sscanf (DirInfo->d_name, "%4s", NewInfo->Signature);
            Info->Next = NewInfo;
            Info = NewInfo;
            Gbl_TableListHead->Instance++;
        }
    }

    closedir (TableDir);
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    OslMapTable
 *
 * PARAMETERS:  Address             - Address of the table in memory
 *              Signature           - Optional ACPI Signature for desired table.
 *                                    Null terminated 4-character string.
 *              Table               - Where a pointer to the mapped table is
 *                                    returned
 *
 * RETURN:      Status; Mapped table is returned if AE_OK.
 *
 * DESCRIPTION: Map entire ACPI table into caller's address space. Also
 *              validates the table and checksum.
 *
 *****************************************************************************/

static ACPI_STATUS
OslMapTable (
    ACPI_SIZE               Address,
    char                    *Signature,
    ACPI_TABLE_HEADER       **Table)
{
    ACPI_TABLE_HEADER       *MappedTable;
    UINT32                  Length;


    /* Map the header so we can get the table length */

    MappedTable = AcpiOsMapMemory (Address, sizeof (ACPI_TABLE_HEADER));
    if (!MappedTable)
    {
        fprintf (stderr, "Could not map table header at 0x%8.8X%8.8X\n",
            ACPI_FORMAT_UINT64 (Address));
        return (AE_BAD_ADDRESS);
    }

    /* Check if table is valid */

    if (!ApIsValidHeader (MappedTable))
    {
        return (AE_BAD_HEADER);
    }

    /* If specified, signature must match */

    if (Signature &&
        !ACPI_COMPARE_NAME (Signature, MappedTable->Signature))
    {
        return (AE_NOT_EXIST);
    }

    /* Map the entire table */

    Length = MappedTable->Length;
    AcpiOsUnmapMemory (MappedTable, sizeof (ACPI_TABLE_HEADER));

    MappedTable = AcpiOsMapMemory (Address, Length);
    if (!MappedTable)
    {
        fprintf (stderr, "Could not map table at 0x%8.8X%8.8X\n",
            ACPI_FORMAT_UINT64 (Address));
        return (AE_NO_MEMORY);
    }

    *Table = MappedTable;

    /* Checksum for RSDP */

    if (!ACPI_STRNCMP (MappedTable->Signature, ACPI_SIG_RSDP,
            sizeof (ACPI_SIG_RSDP) - 1))
    {
        if (MappedTable->Revision)
        {
            if (AcpiTbChecksum ((UINT8 *) MappedTable,
                ACPI_RSDP_XCHECKSUM_LENGTH))
            {
                fprintf (stderr, "Warning: wrong checksum\n");
            }
            else if (AcpiTbChecksum ((UINT8 *) MappedTable,
                ACPI_RSDP_CHECKSUM_LENGTH))
            {
                fprintf (stderr, "Warning: wrong checksum\n");
            }
        }
    }

    /* FACS does not have a checksum */

    if (ACPI_COMPARE_NAME (MappedTable->Signature, ACPI_SIG_FACS))
    {
        return (AE_OK);
    }

    /* Validate checksum for most tables */

    if (AcpiTbChecksum (ACPI_CAST8 (MappedTable), Length))
    {
        fprintf (stderr, "Warning: wrong checksum\n");
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    OslReadTableFromFile
 *
 * PARAMETERS:  TableFile           - File that contains the desired table
 *              FileOffset          - Offset of the table in file
 *              Signature           - Optional ACPI Signature for desired table.
 *                                    A null terminated 4-character string.
 *              Table               - Where a pointer to the table is returned
 *
 * RETURN:      Status; Table buffer is returned if AE_OK.
 *
 * DESCRIPTION: Read a ACPI table from a file.
 *
 *****************************************************************************/

static ACPI_STATUS
OslReadTableFromFile (
    FILE                    *TableFile,
    ACPI_SIZE               FileOffset,
    char                    *Signature,
    ACPI_TABLE_HEADER       **Table)
{
    ACPI_TABLE_HEADER       Header;
    ACPI_TABLE_RSDP         Rsdp;
    ACPI_TABLE_HEADER       *LocalTable;
    UINT32                  TableLength;
    UINT32                  Count;


    /* Read the table header */

    fseek (TableFile, FileOffset, SEEK_SET);

    Count = fread (&Header, 1, sizeof (ACPI_TABLE_HEADER), TableFile);
    if (Count != sizeof (ACPI_TABLE_HEADER))
    {
        fprintf (stderr, "Could not read ACPI table header from file\n");
        return (AE_NOT_FOUND);
    }

    /* Check if table is valid */

    if (!ApIsValidHeader (&Header))
    {
        return (AE_BAD_HEADER);
    }

    /* If signature is specified, it must match the table */

    if (Signature &&
        !ACPI_COMPARE_NAME (Signature, Header.Signature))
    {
        fprintf (stderr, "Incorrect signature: Expecting %4.4s, found %4.4s\n",
            Signature, Header.Signature);
        return (AE_NOT_FOUND);
    }

    /*
     * For RSDP, we must read the entire table, because the length field
     * is in a non-standard place, beyond the normal ACPI header.
     */
    if (ACPI_COMPARE_NAME (Header.Signature, ACPI_SIG_RSDP))
    {
        fseek (TableFile, FileOffset, SEEK_SET);

        Count = fread (&Rsdp, 1, sizeof (ACPI_TABLE_RSDP), TableFile);
        if (Count != sizeof (ACPI_TABLE_RSDP))
        {
            fprintf (stderr, "Error while reading RSDP\n");
            return (AE_NOT_FOUND);
        }

        TableLength = Rsdp.Length;
    }
    else
    {
        TableLength = Header.Length;
    }

    /* Read the entire table into a local buffer */

    LocalTable = calloc (1, TableLength);
    if (!LocalTable)
    {
        fprintf (stderr, 
            "%4.4s: Could not allocate buffer for table of length %X\n",
            Header.Signature, TableLength);
        return (AE_NO_MEMORY);
    }

    fseek (TableFile, FileOffset, SEEK_SET);

    Count = fread (LocalTable, 1, TableLength, TableFile);
    if (Count != TableLength)
    {
        fprintf (stderr, "%4.4s: Error while reading table content\n",
            Header.Signature);
        return (AE_NOT_FOUND);
    }

    /* Validate checksum, except for special tables */

    if (!ACPI_COMPARE_NAME (Header.Signature, ACPI_SIG_S3PT) &&
        !ACPI_COMPARE_NAME (Header.Signature, ACPI_SIG_FACS))
    {
        if (AcpiTbChecksum ((UINT8 *) LocalTable, TableLength))
        {
            fprintf (stderr, "%4.4s: Warning: wrong checksum\n",
                Header.Signature);
        }
    }

    *Table = LocalTable;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    OslGetOverrideTable
 *
 * PARAMETERS:  Signature       - ACPI Signature for desired table. Must be
 *                                a null terminated 4-character string.
 *              Instance        - For SSDTs (0...n)
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer is returned if AE_OK.
 *              AE_NOT_FOUND: A valid table was not found at the address
 *
 * DESCRIPTION: Get a table that was overridden and appears under the
 *              directory OVERRIDE_TABLE_DIR.
 *
 *****************************************************************************/

static ACPI_STATUS
OslGetOverrideTable (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    ACPI_TABLE_HEADER       Header;
    struct stat             FileInfo;
    struct dirent           *DirInfo;
    DIR                     *TableDir;
    FILE                    *TableFile = NULL;
    UINT32                  CurrentInstance = 0;
    UINT32                  Count;
    UINT8                   FoundTable = FALSE;
    char                    TempName[4];
    char                    TableFilename[PATH_MAX];


    /* Open the directory for override tables */

    if (stat (OVERRIDE_TABLE_DIR, &FileInfo) == -1)
    {
        return (AE_NOT_FOUND);
    }

    if (!(TableDir = opendir (OVERRIDE_TABLE_DIR)))
    {
        return (AE_ERROR);
    }

    /* Attempt to find the table in the directory */

    while (!FoundTable && (DirInfo = readdir (TableDir)) != 0)
    {
        /* Ignore meaningless files */

        if (DirInfo->d_name[0] == '.')
        {
            continue;
        }

        if (!ACPI_COMPARE_NAME (DirInfo->d_name, Signature))
        {
            continue;
        }

        if (strlen (DirInfo->d_name) > 4)
        {
            sscanf (DirInfo->d_name, "%[^1-9]%d", TempName, &CurrentInstance);
            if (CurrentInstance != Instance)
            {
                continue;
            }
        }

        /* Get the table filename and open the file */

        FoundTable = TRUE;
        sprintf (TableFilename, "%s/%s", OVERRIDE_TABLE_DIR, DirInfo->d_name);

        TableFile = fopen (TableFilename, "rb");
        if (TableFile == NULL)
        {
            perror (TableFilename);
            return (AE_ERROR);
        }

        /* Read the Table header to get the table length */

        Count = fread (&Header, 1, sizeof (ACPI_TABLE_HEADER), TableFile);
        if (Count != sizeof (ACPI_TABLE_HEADER))
        {
            fclose (TableFile);
            return (AE_ERROR);
        }
    }

    closedir (TableDir);
    if (ACPI_COMPARE_NAME (Signature, ACPI_SIG_SSDT) &&
        !FoundTable)
    {
        return (AE_LIMIT);
    }

    if (!FoundTable)
    {
        return (AE_NOT_FOUND);
    }

    /* There is no physical address for override tables, use zero */

    *Address = 0;
    return (OslReadTableFromFile (TableFile, 0, NULL, Table));
}


/******************************************************************************
 *
 * FUNCTION:    OslGetDynamicSsdt
 *
 * PARAMETERS:  Instance        - For SSDTs (0...n)
 *              Table           - Where a pointer to the table is returned
 *              Address         - Where the table physical address is returned
 *
 * RETURN:      Status; Table buffer is returned if AE_OK.
 *              AE_NOT_FOUND: A valid table was not found at the address
 *
 * DESCRIPTION: Get an SSDT table under directory DYNAMIC_SSDT_DIR.
 *
 *****************************************************************************/

static ACPI_STATUS
OslGetDynamicSsdt (
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    ACPI_TABLE_HEADER       Header;
    struct stat             FileInfo;
    struct dirent           *DirInfo;
    DIR                     *TableDir;
    FILE                    *TableFile = NULL;
    UINT32                  Count;
    UINT32                  CurrentInstance = 0;
    UINT8                   FoundTable = FALSE;
    char                    TempName[4];
    char                    TableFilename[PATH_MAX];


    /* Open the directory for dynamically loaded SSDTs */

    if (stat (DYNAMIC_SSDT_DIR, &FileInfo) == -1)
    {
        return (AE_NOT_FOUND);
    }

    if (!(TableDir = opendir (DYNAMIC_SSDT_DIR)))
    {
        return (AE_ERROR);
    }

    /* Search directory for correct SSDT instance */

    while (!FoundTable && (DirInfo = readdir (TableDir)) != 0)
    {
        /* Ignore meaningless files */

        if (DirInfo->d_name[0] == '.')
        {
            continue;
        }

        /* Check if this table is what we need */

        sscanf (DirInfo->d_name, "%[^1-9]%d", TempName, &CurrentInstance);
        if (CurrentInstance != Instance)
        {
            continue;
        }

        /* Get the SSDT filename and open the file */

        FoundTable = TRUE;
        sprintf (TableFilename, "%s/%s", DYNAMIC_SSDT_DIR, DirInfo->d_name);

        TableFile = fopen (TableFilename, "rb");
        if (TableFile == NULL)
        {
            perror (TableFilename);
            return (AE_ERROR);
        }

        /* Read the Table header to get the table length */

        Count = fread (&Header, 1, sizeof (ACPI_TABLE_HEADER), TableFile);
        if (Count != sizeof (ACPI_TABLE_HEADER))
        {
            fclose (TableFile);
            return (AE_ERROR);
        }
    }

    closedir (TableDir);
    if (CurrentInstance != Instance)
    {
        return (AE_LIMIT);
    }

    /* Address of overridden table is not supported by Linux currently */

    *Address = 0;
    return (OslReadTableFromFile (TableFile, Header.Length, NULL, Table));
}