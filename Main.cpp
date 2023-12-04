#include <cstdio>
#include <string>
#include <vector>

//------------------------------------------------------------------------------

static unsigned short GetFSDSectorSize(unsigned char Index)
{
    switch (Index)
    {
        case 0:
        default:
            return 128;

        case 1:
            return 256;

        case 2:
            return 512;

        case 3:
            return 1024;

        case 4:
            return 2048;
    }
}

//------------------------------------------------------------------------------

static void Dump(const unsigned char* pData, int Length)
{
    const int BytesPerLine = 16;

    int Offset = 0;

    while (Length > 0)
    {
        int i;

        for (i = 0; i < BytesPerLine && i < Length; i++)
        {
            printf("%02X ", pData[Offset + i]);
        }

        printf(" ");

        for (i = 0; i < BytesPerLine && i < Length; i++)
        {
            printf("%c", isprint(pData[Offset + i]) ? pData[Offset + i] : '.');
        }

        printf("\n");

        Length -= BytesPerLine;
        Offset += BytesPerLine;
    }
}

//------------------------------------------------------------------------------

static void DumpFSDFile(const char *FileName)
{
    FILE *infile = fopen(FileName, "rb");

    if (infile == nullptr)
    {
        fprintf(stderr, "Failed to open file: %s\n", FileName);
        return;
    }

    const int Head = 0;

    unsigned char FSDheader[8]; // FSD - Header information
    fread(FSDheader, 1, 8, infile); // Skip FSD Header

    printf("FSD Header: ");
    Dump(FSDheader, 8);

    printf("\n");

    std::string disctitle;
    char dtchar = 1;

    while (dtchar != 0)
    {
        dtchar = fgetc(infile);
        disctitle = disctitle + dtchar;
    }

    printf("Disc Title: %s\n", disctitle.c_str());

    int TotalTracks = fgetc(infile); // Read number of tracks on disk image

    printf("Tracks: %d\n\n", TotalTracks);

    for (int Track = 0; Track < TotalTracks; Track++)
    {
        unsigned char fctrack = fgetc(infile); // Read current track details
        unsigned char SectorsPerTrack = fgetc(infile); // Read number of sectors on track

        printf("Track: %d\n", Track);
        // printf("fctrack: %d\n", fctrack);
        printf("Sectors: %d\n", SectorsPerTrack);

        if (SectorsPerTrack > 0) // i.e., if the track is formatted
        {
            unsigned char TrackIsReadable = fgetc(infile); // Is track readable?

            printf("Readable: %d\n\n", TrackIsReadable);

            for (int Sector = 0; Sector < SectorsPerTrack; Sector++)
            {
                printf("Track %d, Sector %d\n", Track, Sector);

                unsigned char LogicalTrack = fgetc(infile); // Logical track ID

                printf("Logical Track ID: %d\n", LogicalTrack);

                unsigned char HeadNum = fgetc(infile); // Head number

                printf("Head Number: %d\n", HeadNum);

                unsigned char LogicalSector = fgetc(infile); // Logical sector ID

                printf("Logical Sector ID: %d\n", LogicalSector);

                unsigned char FRecLength = fgetc(infile); // Reported length of sector

                printf("Reported Sector Length: %d (%d bytes)\n", FRecLength, GetFSDSectorSize(FRecLength));

                if (TrackIsReadable == 255)
                {
                    unsigned char FPRecLength = fgetc(infile); // Real size of sector, can be misreported as copy protection
                    unsigned short FSectorSize = GetFSDSectorSize(FPRecLength);

                    printf("Actual Sector Length: %d (%d bytes)\n", FPRecLength, FSectorSize);

                    unsigned char FErr = fgetc(infile); // Error code when sector was read

                    printf("Sector Error: %02X\n\n", FErr);

                    std::vector<unsigned char> Data(FSectorSize);

                    fread(&Data[0], 1, FSectorSize, infile);

                    Dump(&Data[0], Data.size());

                    printf("\n");
                }
            }
        }
        else
        {
            printf("\n");
        }
    }

    fclose(infile);
}

//------------------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    const char* FileName = argv[1];

    if (FileName != nullptr)
    {
        DumpFSDFile(FileName);
    }
    else
    {
        fprintf(stderr, "Usage: DumpFSD <filename>\n");
    }
}
