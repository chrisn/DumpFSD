#include <cstdio>
#include <map>
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

    std::map<unsigned char, int> Errors;
    std::map<int, int> SectorSizes;
    bool HasNonReadableTracks = false;
    bool HasNonDefaultTrackIDs = false;
    bool HasNonDefaultSectorIDs = false;

    const int Head = 0;

    unsigned char FSDHeader[3];
    fread(FSDHeader, 1, 3, infile); // Read FSD Header

    if (FSDHeader[0] != 'F' || FSDHeader[1] != 'S' || FSDHeader[2] != 'D')
    {
        fprintf(stderr, "Missing FSD file header\n");
        fclose(infile);

        return;
    }

    // See https://github.com/richtw1/fsd2fdi/blob/master/src/fsdimage.cpp

    unsigned char Info[5];
    fread(Info, 1, 5, infile);

    int Day = Info[0] >> 3;
    int Month = Info[2] & 0x0F;
    int Year = ((Info[0] & 0x07) << 8) | Info[1];

    int CreatorID = Info[2] >> 4;
    int Release = ((Info[4] >> 6) << 8) | Info[3];

    printf("Date: %02d-%02d-%d\n", Day, Month, Year);
    printf("CreatorID: %d\n", CreatorID);
    printf("Release: %d\n", Release);

    std::string Title;
    char Char = 1;

    while (Char != '\0')
    {
        Char = fgetc(infile);
        Title += Char;
    }

    printf("Disc Title: %s\n", Title.c_str());

    int TotalTracks = fgetc(infile); // Read number of tracks on disk image

    printf("Tracks: %d\n\n", TotalTracks);

    for (int Track = 0; Track < TotalTracks; Track++)
    {
        unsigned char TrackNumber = fgetc(infile); // Read current track details
        unsigned char SectorsPerTrack = fgetc(infile); // Read number of sectors on track

        printf("Track: %d\n", TrackNumber);
        printf("Sectors: %d\n", SectorsPerTrack);

        if (SectorsPerTrack > 0) // i.e., if the track is formatted
        {
            unsigned char TrackIsReadable = fgetc(infile); // Is track readable?

            printf("Readable: %d\n\n", TrackIsReadable);

            for (int Sector = 0; Sector < SectorsPerTrack; Sector++)
            {
                printf("Track %d, Sector %d\n", TrackNumber, Sector);

                unsigned char LogicalTrack = fgetc(infile); // Logical track ID

                printf("Logical Track ID: %d\n", LogicalTrack);

                if (LogicalTrack != TrackNumber)
                {
                    HasNonDefaultTrackIDs = true;
                }

                unsigned char HeadNum = fgetc(infile); // Head number

                printf("Head Number: %d\n", HeadNum);

                unsigned char LogicalSector = fgetc(infile); // Logical sector ID

                printf("Logical Sector ID: %d\n", LogicalSector);

                if (LogicalSector != Sector)
                {
                    HasNonDefaultSectorIDs = true;
                }

                unsigned char SectorSize = fgetc(infile); // Reported length of sector

                printf("Reported Sector Length: %d (%d bytes)\n", SectorSize, GetFSDSectorSize(SectorSize));

                if (TrackIsReadable == 255)
                {
                    SectorSize = fgetc(infile); // Real size of sector, can be misreported as copy protection
                    unsigned short ActualSectorSize = GetFSDSectorSize(SectorSize);

                    printf("Actual Sector Length: %d (%d bytes)\n", SectorSize, ActualSectorSize);

                    SectorSizes[SectorSize]++;

                    unsigned char Error = fgetc(infile); // Error code when sector was read

                    printf("Sector Error: %02X\n\n", Error);

                    Errors[Error]++;

                    std::vector<unsigned char> Data(ActualSectorSize);

                    fread(&Data[0], 1, ActualSectorSize, infile);

                    Dump(&Data[0], Data.size());

                    printf("\n");
                }
                else
                {
                    HasNonReadableTracks = true;

                    printf("\n");
                }
            }
        }
        else
        {
            printf("\n");
        }
    }

    if (Errors.size() > 0)
    {
        printf("\nSummary\n-------\n\n");
        printf("Non-Default Track IDs: %s\n", HasNonDefaultTrackIDs ? "Yes" : "No");
        printf("Non-Default Sector IDs: %s\n", HasNonDefaultSectorIDs ? "Yes" : "No");
        printf("Non-Readable Tracks: %s\n", HasNonReadableTracks ? "Yes" : "No");

        printf("\nSector Sizes:\n\n");

        for (const auto& Entry : SectorSizes)
        {
            printf("%d bytes: %d\n", Entry.first, Entry.second);
        }

        printf("\nSector Error Codes:\n\n");

        for (const auto& Entry : Errors)
        {
            printf("%02X: %d\n", Entry.first, Entry.second);
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
