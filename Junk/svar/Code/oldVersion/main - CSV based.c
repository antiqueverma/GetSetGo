#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdint.h"

#define MAX_CELL_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define LINE_WIDTH      100

typedef enum state_t
{
    PARSE_INIT,
    GET_LINE,
    PARSE_LINE,
    WRITE_LINE,
    WRITE_RAM_FILE,
    BLANK_FIELD,
    PARSE_TERMINATE
} state_t;

typedef enum sectionList_t
{
    SETUP_INFO = 1,
    CONTEXT_DATA,
    SEGMENT,        // Segment should always be the last enum value
    
} sectionList_t;
char line[MAX_LINE_LENGTH];
char paramValue[MAX_CELL_LENGTH]; // Array to store the value of the next cell after <SECTION>
const char *storageTypes[] = {"RAM_ONLY", "EEPROM_ONLY", "SHADOWED_EEPROM"};

typedef struct setupData_t{
    uint8_t segCount;
    uint16_t pageSize;
    uint8_t packedStorage_en;
    
    //Below variables ARE ONLY needed for formatting of output files
    uint8_t addBaseHexEn;
    uint8_t segTypePrfxEn;
    uint8_t segNamePrfxEn;
}setupData_t;

typedef struct segData_t{
    char flavor[20];
    uint32_t offAdd;
    char segPrefix[5];
    uint16_t writeTime;
    uint16_t size;
    uint32_t  addOffset;

}segData_t;

int main() 
{
    state_t parsingState = PARSE_INIT;
    uint8_t prvState = 0xFF;
    FILE *file_CSV = NULL;
    FILE *file_Add_H = NULL;
    FILE *file_Ram_C = NULL;
    FILE *file_Ram_txt = NULL;
    
    setupData_t setup;
    segData_t  segment[20];

    uint8_t i=0;
    for(i=0 ; i<20 ; i++)
    {
       
        strcpy(segment[i].flavor, "SHADOWED_EEPROM");
        segment[i].writeTime = 60;
        segment[i].offAdd = 256;
        segment[i].size = 200;
    }


    uint8_t sectionCtr = 0;
    uint8_t sectionPhase = 0;
    uint16_t lineCtr = 0;
    uint32_t AddCtr = 0;
    int padding = 0;

    while(1)
    {
        if(prvState != parsingState)
        {
            prvState = parsingState;
            printf("\nState => %d\t",parsingState);
        }
        switch(parsingState)
        {
            case PARSE_INIT:
            {
                printf("\nstate> PARSE_INIT\n");
                file_CSV = fopen("input_files/SVAR.csv", "r");
                if (file_CSV == NULL) 
                {
                    printf("CSV not found");
                    parsingState = PARSE_TERMINATE;
                    break;
                }
                
                file_Add_H = fopen("svar/svar_add.h", "w+");
                if (file_Add_H == NULL) 
                {
                    printf("svar_add not found");
                    parsingState = PARSE_TERMINATE;
                    break;
                }

                file_Ram_txt = fopen("input_files/svar_ram_template.txt", "r");
                if (file_Ram_txt == NULL) 
                {
                    printf("svar_ram_template.txt not found");
                    parsingState = PARSE_TERMINATE;
                    break;
                }

                file_Ram_C = fopen("svar/svar_ram.c", "w+");
                if (file_Ram_C == NULL) 
                {
                    printf("svar_ram.c not found");
                    parsingState = PARSE_TERMINATE;
                    break;
                }
                parsingState = GET_LINE;
                break;
            }
            case GET_LINE:
            {
                static uint8_t blankCtr = 0;
                if (fgets(line, sizeof(line), file_CSV) == NULL)
                {
                    parsingState = BLANK_FIELD;
                    break;
                }
                lineCtr++;
                line[strcspn(line, "\r\n")] = '\0';
                // printf("\n\t\tline=\t %s",line);
                parsingState = PARSE_LINE;
                break;
            }
            case PARSE_LINE:
            {
                // Check if the first cell is <SECTION>
                if (strstr(line, "<SECTION>") != NULL)
                {
                    char *token = strtok(line, ","); // Skip first cell
                    token = strtok(NULL, ",");       // Get next cell
                    if (token != NULL) 
                    {
                        uint8_t i;
                        sectionCtr++;

                        fprintf(file_Add_H, "\n/");
                        for(i = 0; i<LINE_WIDTH-2 ; i++)
                            fputc('*', file_Add_H);
                        fprintf(file_Add_H, "/\n");

                        if(sectionCtr == SETUP_INFO)
                        {
                            sectionPhase++;
                        }
                        else 
                        {
                            fprintf(file_Add_H, "\n/");
                            for(i = 0; i<LINE_WIDTH-2 ; i++)
                                fputc('*', file_Add_H);
                            fprintf(file_Add_H, "/\n");
                            sectionPhase = 0;
                        }
                        
                        strncpy(paramValue, token, MAX_CELL_LENGTH - 1);
                        paramValue[MAX_CELL_LENGTH - 1] = '\0'; // Null-terminate
                        printf("\n\t\tSection Name: %s", paramValue);

                        padding = LINE_WIDTH;
                        padding -= strlen(paramValue);
                        padding -= 16;
                        // Write to file
                        fprintf(file_Add_H, "/*\tSECTION:  ");
                        fprintf(file_Add_H, "%s", paramValue);
                        fprintf(file_Add_H, "%*s*/", padding, "");

                        parsingState = GET_LINE;    //Jump to next line
                        break;
                    } /**/
                    else
                    {
                        printf("\nERROR: No value found after <SECTION>.\n");
                        parsingState = PARSE_TERMINATE;
                    }
                }
                else if(sectionCtr == SETUP_INFO)
                {
                    
                }
                else if(sectionCtr == CONTEXT_DATA)
                {
                    ;
                }
                else //if(sectionCtr == SEGMENT)
                {
                    ;
                }
                parsingState = WRITE_LINE;
                break;
            }
            case WRITE_LINE:
            {
                char *token = strtok(line, ","); // get first cell
                if(sectionCtr == SETUP_INFO)
                {
                    padding = LINE_WIDTH;
                    padding -= strlen(token);

                    fprintf(file_Add_H, "\n/*\t%s:  ", token);
                    padding -= 9;

                    token = strtok(NULL, ",");       // Get next cell
                    if(token != NULL)
                    {
                        padding -= strlen(token);
                        // fprintf(outputFile, "%s", token);
                        fprintf(file_Add_H, "%s", token);
                        fprintf(file_Add_H, "%*s*/", padding, "");
                    }
                    else
                    {
                        parsingState = BLANK_FIELD;
                        break;
                    }
                }
                else if(sectionCtr == CONTEXT_DATA)
                {
                    
                    padding = 55;
                    fprintf(file_Add_H, "\n#define\t");
                    padding -= 9;
                    fprintf(file_Add_H, " %s", token);
                    padding -= strlen(token);
                    
                    token = strtok(NULL, ",");       // Get next cell
                    if(token != NULL)
                    {
                        fprintf(file_Add_H, "%*s", padding, token);
                        
                        
                        static uint8_t paramCtr = 0;
                        uint32_t value = 0;
                        char *endPtr = NULL;
                        value = (uint32_t)strtoul(token, &endPtr, 10);
                        switch(paramCtr)
                        {
                            case 0:
                                setup.segCount = value;
                                break;
                            case 1:
                                setup.pageSize = value;
                                break;
                            case 2:
                                setup.packedStorage_en = value;
                                break;
                            case 3:
                                setup.segTypePrfxEn = value;
                                break;
                            case 4:
                                setup.segNamePrfxEn = value;
                                break;
                            case 5:
                                setup.addBaseHexEn = value;
                                break;
                        }
                        paramCtr++;
                    }
                    else
                    {
                        parsingState = BLANK_FIELD;
                        break;
                    }
                }
                else if(sectionCtr >= SEGMENT)
                {
                    // static uint8_t prvSegment = 0;
                    // if(prvSegment != sectionCtr)
                    // {
                    //     prvSegment = sectionCtr;
                    //     fprintf(file_Add_H, "\n#define  SVAR_SEGMENT%d_ADDRESS_OFFSET",sectionCtr-SEGMENT);
                    //     padding = 55;
                    // }
                    char *endPtr = NULL;
                    uint32_t value = 0;
                    padding = 55;
                    fprintf(file_Add_H, "\n#define  ");
                    padding -= 9;
                    
                    padding -= strlen(token);
                    fprintf(file_Add_H, "%s", token);

                    token = strtok(NULL, ",");       // Get next cell
                    if(token == NULL)
                    {
                        parsingState = BLANK_FIELD;
                        break;
                    }

                    value = (uint32_t)strtoul(token, &endPtr, 10);
                    if((value == 0)||(value > 20))
                    {
                        printf("\n\nInvalid numeric value at = %d",lineCtr); 
                    }
                    // fprintf(outputFile, " %d\t\t", AddCtr);
                    fprintf(file_Add_H, "%*d", padding, AddCtr);
                    AddCtr += value;

                    token = strtok(NULL, ",");       // Get next cell
                    if(token == NULL)
                    {
                        parsingState = BLANK_FIELD;
                        break;
                    }
                    fprintf(file_Add_H, "\t\t// %s", token);
                }    
                parsingState = GET_LINE; 
                break;
            }
            
            case WRITE_RAM_FILE:
            {   printf("A- ");
                fseek(file_Ram_C, 0, SEEK_SET); // Start at the beginning
                char ch;
                uint16_t paramId = 0;

                uint16_t ctr=0;

                while ((ch = fgetc(file_Ram_txt)) != EOF)
                {
                    if (ch == '$')
                    {
                        // Replace $ with param value using switch
                        switch (paramId)
                        {
                            case 0: 
                            case 4: 
                            {   
                                fprintf(file_Ram_C, "%d", setup.segCount); // %hu is the format specifier for unsigned short (uint16_t)
                                break;
                            }
                            case 1: 
                            {
                                fprintf(file_Ram_C, "%d", setup.packedStorage_en); // %hu is the format specifier for unsigned short (uint16_t)
                                break;
                            }
                            case 2: 
                            {
                                fprintf(file_Ram_C, "%d", setup.pageSize); // %hu is the format specifier for unsigned short (uint16_t)
                                break;
                            }
                            case 5:
                            {
                                uint8_t i=0;
                                uint32_t addOffset = 0; // Start offset at 0
                                
                                fprintf(file_Ram_C, "\t");
                                for(i=0 ; i<setup.segCount ; i++)
                                {
                                    // Write index
                                    fprintf(file_Ram_C, "[%d] = {", i);

                                    // Write each field in robust format
                                    fprintf(file_Ram_C, ".addOffset = %*d,\t", 5,addOffset);
                                    fprintf(file_Ram_C, ".nvmOffset = %*d,\t", 5,segment[i].offAdd);
                                    fprintf(file_Ram_C, ".writeTime = %*d,\t", 5,segment[i].writeTime);
                                    fprintf(file_Ram_C, ".buffer = seg%d_buffer,\t", i);
                                    fprintf(file_Ram_C, ".flavor = %s ", segment[i].flavor);
                                    
                                    // Calculate next segment's addOffset
                                    addOffset += segment[i].size;

                                    // Write svar storage flavor and EEPROM offset
                                    // fprintf(file_Ram_C, "%s,", 
                                    //         segment[i].flavor);

                                    // fprintf(file_Ram_C, "%*d,", 
                                    //         15, 
                                    //         segment[i].offAdd);
                                         
                                    // // Write EEPROM write interval
                                    // padding = 15; 
                                    // fprintf(file_Ram_C, "%*d,", padding, segment[i].writeTime);

                                    // fprintf(file_Ram_C, "\t\t  seg%d_buffer", i);
                                    // Line closing
                                    fprintf(file_Ram_C, "},\n\t");
                                }
                                break;
                            }
                            case 3:
                            {
                                uint8_t i=0;
                                for(i=0 ; i<setup.segCount ; i++)
                                {
                                    // Write data type
                                    fprintf(file_Ram_C, "static uint8_t seg%d_buffer[%d];\n", i, segment[i].size);
                                }
                                break;
                            }

                            default:
                            {
                                break;
                            }
                        }
                        paramId++;
                    }
                    else
                    {
                        fputc(ch, file_Ram_C); // Directly copy other chars
                    }
                }
                parsingState = PARSE_TERMINATE;
                break;
            }
            case BLANK_FIELD:
            {
                printf("\n\nBlank Cell in CSV line number = %d",lineCtr); 
                parsingState = PARSE_TERMINATE;
                parsingState = WRITE_RAM_FILE;
                break;
            }
            case PARSE_TERMINATE:
            default:
            {
                printf("\nTerminating Process!");
                fclose(file_CSV);
                fclose(file_Add_H);
                fclose(file_Ram_C);
                fclose(file_Ram_txt);
                return 1;
                break;
            }
        }
    }

    return 0;    
}
