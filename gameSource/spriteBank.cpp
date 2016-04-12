
#include "spriteBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SpriteRecord **idMap;


static StringTree tree;



static int numFiles;
static File **childFiles;

static int currentFile;


static SimpleVector<SpriteRecord*> records;
static int maxID;


static File spritesDir( NULL, "sprites" );


void initSpriteBankStart() {
    maxID = 0;
    
    numFiles = 0;
    currentFile = 0;

    if( spritesDir.exists() && spritesDir.isDirectory() ) {

        childFiles = spritesDir.getChildFiles( &numFiles );
        }
    }




// expands true regions by making neighbor pixels true also
void expandMap( char *inMap, int inW, int inH ) {
    int numPixels = inW * inH;
    
    char *copy = new char[ numPixels ];
    
    memcpy( copy, inMap, numPixels );
    
    // avoid edges
    for( int y = 1; y < inH-1; y++ ) {
        for( int x = 1; x < inW-1; x++ ) {
            int index = y * inW + x;
            
            if( copy[index] ) {
                // make neighbors true also

                inMap[index-1] = true;
                inMap[index+1] = true;

                inMap[index-inW] = true;
                inMap[index+inW] = true;                
                }
            }
        }
    
    delete [] copy;
    }




float initSpriteBankStep() {
    
    if( currentFile == numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;
            
    if( !childFiles[i]->isDirectory() ) {
                
        char *fileName = childFiles[i]->getFileName();
    
        // skip all non-tga files
        if( strstr( fileName, ".tga" ) != NULL ) {
            
            // a tga file!
            
            char *fullName = childFiles[i]->getFullFileName();
                            
            printf( "Loading sprite from path %s\n", fullName );
                            

            RawRGBAImage *spriteImage = readTGAFileRawBase( fullName );

            if( spriteImage != NULL && spriteImage->mNumChannels != 4 ) {
                printf( "Sprite at %s not a 4-channel image, "
                        "failed to load.\n",
                        fullName );
                delete spriteImage;
                spriteImage = NULL;
                }

            delete [] fullName;

                            
            if( spriteImage != NULL ) {
                SpriteRecord *r = new SpriteRecord;

                        
                r->sprite =
                    fillSprite( spriteImage->mRGBABytes, spriteImage->mWidth,
                                spriteImage->mHeight );
                
                r->w = spriteImage->mWidth;
                r->h = spriteImage->mHeight;
                        
                int numPixels = r->w * r->h;
                r->hitMap = new char[ numPixels ];

                memset( r->hitMap, 1, numPixels );
                        
                
                int numBytes = numPixels * 4;
                
                unsigned char *bytes = spriteImage->mRGBABytes;
                
                // alpha is 4th byte
                int p=0;
                for( int b=3; b<numBytes; b+=4 ) {
                    if( bytes[b] < 64 ) {
                        r->hitMap[p] = 0;
                        }
                    p++;
                    }
                                 
                for( int e=0; e<3; e++ ) {    
                    expandMap( r->hitMap, r->w, r->h );
                    }
                        
                delete spriteImage;
                      


                r->id = 0;
                                
                sscanf( fileName, "%d.tga", &( r->id ) );
                
                char *textFileName = autoSprintf( "%d.txt", r->id );
                
                File *textFile = spritesDir.getChildFile( textFileName );
                
                delete [] textFileName;
                
                char *contents = textFile->readFileContents();
                
                delete textFile;
                
                r->tag = NULL;

                if( contents != NULL ) {
                    
                    SimpleVector<char *> *tokens = tokenizeString( contents );
                    int numTokens = tokens->size();
                    
                    if( numTokens >= 2 ) {
                        
                        r->tag = 
                            stringDuplicate( tokens->getElementDirect( 0 ) );
                        
                        int mult;
                        sscanf( tokens->getElementDirect( 1 ),
                                "%d", &mult );
                        
                        if( mult == 1 ) {
                            r->multiplicativeBlend = true;
                            }
                        else {
                            r->multiplicativeBlend = false;
                            }
                        }

                    tokens->deallocateStringElements();
                    delete tokens;
                    
                    delete [] contents;
                    }
                
                if( r->tag == NULL ) {
                    r->tag = stringDuplicate( "tag" );
                    r->multiplicativeBlend = false;
                    }
                
                r->maxD = getSpriteWidth( r->sprite );
                if( getSpriteHeight( r->sprite ) > r->maxD ) {
                    r->maxD = getSpriteHeight( r->sprite );
                            }
                records.push_back( r );

                if( maxID < r->id ) {
                    maxID = r->id;
                    }
                }
            }
        delete [] fileName;
        }
                
    delete childFiles[i];


    currentFile ++;
    return (float)( currentFile ) / (float)( numFiles );
    }
 

void initSpriteBankFinish() {    

    delete [] childFiles;
    
    mapSize = maxID + 1;
    
    idMap = new SpriteRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        SpriteRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        char *lower = stringToLowerCase( r->tag );
        
        tree.insert( lower, r );
        
        delete [] lower;
        }

    printf( "Loaded %d tagged sprites from sprites folder\n", numRecords );
    }




static void freeSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
         
            freeSprite( idMap[inID]->sprite );
            
            char *lower = stringToLowerCase( idMap[inID]->tag );
            
            tree.remove( lower, idMap[inID] );

            delete [] lower;

            delete [] idMap[inID]->tag;
                        
            if( idMap[inID]->hitMap != NULL ) {
                delete [] idMap[inID]->hitMap;    
                }
            
            
            delete idMap[inID];
            idMap[inID] = NULL;
            
            return;
            }
        }
    }




void freeSpriteBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            freeSprite( idMap[i]->sprite );
            delete [] idMap[i]->tag;

             if( idMap[i]->hitMap != NULL ) {
                delete [] idMap[i]->hitMap;    
                }

            delete idMap[i];
            }
        }

    delete [] idMap;
    }




SpriteRecord *getSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        return idMap[inID];
        }
    else {
        return NULL;
        }
    }



char getUsesMultiplicativeBlending( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID]->multiplicativeBlend;
            }
        }
    return false;
    }
    


SpriteHandle getSprite( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID]->sprite;
            }
        }
    return NULL;
    }

    


// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    char *lower = stringToLowerCase( inSearch );
    
    int numTotalMatches = tree.countMatches( lower );
        
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    SpriteRecord **results = new SpriteRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lower, inNumToSkip, numToGet, (void**)results );
    
    delete [] lower;

    return results;
    }



int addSprite( const char *inTag, SpriteHandle inSprite,
               Image *inSourceImage,
               char inMultiplicativeBlending ) {

    int maxD = inSourceImage->getWidth();
    
    if( maxD < inSourceImage->getHeight() ) {
        maxD = inSourceImage->getHeight();
        }
    
    int newID = -1;


    // add it to file structure
    File spritesDir( NULL, "sprites" );
            
    if( !spritesDir.exists() ) {
        spritesDir.makeDirectory();
        }
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {
                
                
        int nextSpriteNumber = 1;
                
        File *nextNumberFile = 
            spritesDir.getChildFile( "nextSpriteNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextSpriteNumber );
                
                delete [] nextNumberString;
                }
            }
                
                    
            
        const char *printFormatTGA = "%d.tga";
        const char *printFormatTXT = "%d.txt";
        
        char *fileNameTGA = autoSprintf( printFormatTGA, nextSpriteNumber );
        char *fileNameTXT = autoSprintf( printFormatTXT, nextSpriteNumber );
            
        newID = nextSpriteNumber;

        File *spriteFile = spritesDir.getChildFile( fileNameTGA );
            
        TGAImageConverter tga;
            
        FileOutputStream stream( spriteFile );
        
        tga.formatImage( inSourceImage, &stream );
                    
        delete [] fileNameTGA;
        delete spriteFile;

        File *metaFile = spritesDir.getChildFile( fileNameTXT );

        int multFlag = 0;
        if( inMultiplicativeBlending ) {
            multFlag = 1;
            }
        
        char *metaContents = autoSprintf( "%s %d", inTag, multFlag );

        metaFile->writeToFile( metaContents );
        
        delete [] metaContents;
        delete [] fileNameTXT;
        delete metaFile;

        nextSpriteNumber++;
        

                
        char *nextNumberString = autoSprintf( "%d", nextSpriteNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        }
    
    if( newID == -1 ) {
        // failed to save it to disk
        return -1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        

        
        SpriteRecord **newMap = new SpriteRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(SpriteRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    SpriteRecord *r = new SpriteRecord;
    
    r->id = newID;
    r->sprite = inSprite;
    r->tag = stringDuplicate( inTag );
    r->maxD = maxD;
    r->multiplicativeBlend = inMultiplicativeBlending;


    r->w = inSourceImage->getWidth();
    r->h = inSourceImage->getHeight();
    
    int numPixels = r->w * r->h;
    r->hitMap = new char[ numPixels ];
    
    memset( r->hitMap, 1, numPixels );
    
    if( inSourceImage->getNumChannels() == 4 ) {
        
        double *a = inSourceImage->getChannel(3);
        
        for( int p=0; p<numPixels; p++ ) {
            if( a[p] < 0.25 ) {
                r->hitMap[p] = 0;
                }
            }
        }
    
    for( int e=0; e<3; e++ ) {    
        expandMap( r->hitMap, r->w, r->h );
        }
    
    
    // delete old
    freeSpriteRecord( newID );
    
    idMap[newID] = r;
    
    char *lower = stringToLowerCase( inTag );
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;

    return newID;
    }



void deleteSpriteFromBank( int inID ) {
    SpriteRecord *r = idMap[ inID ];

    
    File spritesDir( NULL, "sprites" );
    
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {                
                    
        File *tagDir = spritesDir.getChildFile( r->tag );
        
        if( tagDir->exists() && tagDir->isDirectory() ) {
            
            const char *printFormat = "%d.tga";
            
            if( r->multiplicativeBlend ) {
                printFormat = "m%d.tga";
                }

            char *fileName = autoSprintf( printFormat, inID );
            
            File *spriteFile = tagDir->getChildFile( fileName );
            
            spriteFile->remove();
            
            delete [] fileName;
            delete spriteFile;
            
            tagDir->remove();
            }
        delete tagDir;
        }
    
    
    freeSpriteRecord( inID );
    }




char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            int pixX = inXCenterOffset + idMap[inID]->w / 2;
            int pixY = - inYCenterOffset + idMap[inID]->h / 2;
            
            if( pixX >=0 && pixX < idMap[inID]->w 
                &&
                pixY >=0 && pixY < idMap[inID]->h ) {

                int pixI = idMap[inID]->w * pixY + pixX;
                
                return idMap[inID]->hitMap[ pixI ];
                }
            }
        }
    
    return false;
    }



