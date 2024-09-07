#include <color.hpp>
#include <iostream>
#include <filesystem>
#include <format>
#include <string>
#include <FreeImage.h>

namespace fs = std::filesystem;

std::string SaveFolderDir{ "./ConvertedTextures" };
bool GenerateSpecFile = true;
bool RemoveBumpFromFilename = true;
bool CloseTheProgramWithoutPause = true;

std::string replaceBump(const std::string& input, std::string newWord) {
    std::string result = input;
    size_t pos = result.find("bump");
    while (pos != std::string::npos) {
        result.replace(pos, 4, newWord);
        pos = result.find("bump", pos + 1);
    }
    return result;
}

#define default_widht 48
// style text print
void PrintStyleHeader(const std::string& text, int width = default_widht) {
    int padding = (width - text.length()) / 2;

    std::cout << hue::grey;

    for (int i = 0; i < padding; ++i) {
        std::cout << "#";
    }

    if (text.length() % 2 != 0) 
    {
        std::cout << "#";
    }

    std::cout << " " << text << " ";

    for (int i = 0; i < padding; ++i) {
        std::cout << "#";
    }
    std::cout << hue::reset <<std::endl;
}
void CloseWithPause()
{
    FreeImage_DeInitialise();
    CloseTheProgramWithoutPause = false;
}

void ProcessFile(std::string filename, int fileNum, int maxNum)
{
    fs::path file(filename);

    std::string sFileNum = std::format("[{}/{}]", fileNum, maxNum);

    if (!fs::exists(file))
    {
        std::cout << dye::red(sFileNum + " The file could not be found. [" + filename + "]") << std::endl;
        CloseWithPause();
        return;
    }

    std::string onlyFileName = file.filename().string();
    size_t lastDotPos = onlyFileName.find_last_of('.');
    if (lastDotPos != std::string::npos) {
        onlyFileName = onlyFileName.substr(0, lastDotPos);
    }

    if (RemoveBumpFromFilename) {
        onlyFileName = replaceBump(onlyFileName, "");
    }

    FIBITMAP* image = FreeImage_Load(FIF_DDS, file.string().c_str());

    if (!image) {
        std::cout << dye::red(sFileNum + " The file could not be opened. [" + filename + "]\nIf you are using a dds file, make sure that it is not BC7") << std::endl;
        CloseWithPause();
        return;
    }

    int width = FreeImage_GetWidth(image);
    int height = FreeImage_GetHeight(image);
    auto bpp = FreeImage_GetBPP(image);

    PrintStyleHeader(sFileNum);
    std::cout << sFileNum << " File: "<< file.filename() << " [" << width << "x" << height << "] (bpp: " << bpp << ")" << std::endl;


#pragma region Specular

    if (GenerateSpecFile)
    {
        FIBITMAP* Spec = FreeImage_GetChannel(image, FICC_RED);

        if (!fs::exists(SaveFolderDir))
            fs::create_directory(SaveFolderDir);

        std::cout << sFileNum << " Saving: Specular..." << std::endl;

        if (!FreeImage_Save(FIF_TARGA, Spec, std::format("{}/{}{}Spec.tga", SaveFolderDir, onlyFileName, (onlyFileName[onlyFileName.size()-1] == '_') ? "" : "_").c_str(), 0)) {
            std::cout << dye::red(sFileNum + " Error saving the Spec file.") << std::endl;
            CloseWithPause();
            return;
        }
        FreeImage_Unload(Spec);
        std::cout << dye::green(sFileNum + " The Spec is saved.") << std::endl;
    }
#pragma endregion
#pragma region Normal
    FIBITMAP* Normal = FreeImage_ConvertTo24Bits(FreeImage_Clone(image));

    for (unsigned int y = 0; y < FreeImage_GetHeight(Normal); ++y) {
        for (unsigned int x = 0; x < FreeImage_GetWidth(Normal); ++x) {
            RGBQUAD pixelColor;
            FreeImage_GetPixelColor(Normal, x, y, &pixelColor);
            std::swap(pixelColor.rgbBlue, pixelColor.rgbGreen);
            FreeImage_SetPixelColor(Normal, x, y, &pixelColor);
        }
    }
    FIBITMAP* alpha = FreeImage_GetChannel(image, FICC_ALPHA);
    FreeImage_SetChannel(Normal, alpha, FICC_RED);
    FreeImage_Unload(alpha);

    std::cout << sFileNum << " Saving: Normal..." << std::endl;

    if (!FreeImage_Save(FIF_TARGA, Normal, std::format("{}/{}{}Normal.tga", SaveFolderDir, onlyFileName, (onlyFileName[onlyFileName.size() - 1] == '_') ? "" : "_").c_str(), 0)) {
        std::cout << dye::red(" Error saving the Normal file.") << std::endl;
        CloseWithPause();
        return;
    }

    FreeImage_Unload(Normal);
    std::cout << dye::green(sFileNum + " The Normal is saved.") << std::endl<< std::endl;
#pragma endregion
    FreeImage_Unload(image);
}

void Settings_Initialise(std::string file)
{
    PrintStyleHeader("Program Settings");
    const char* section = "Settings";

    if (fs::exists(file))
    {
        char buffer[256];
         GetPrivateProfileString(section, "RemoveBumpFromFilename", "", buffer, sizeof(buffer), file.c_str());
         RemoveBumpFromFilename = (buffer[0] == '1');
         GetPrivateProfileString(section, "GenerateSpecFile", "", buffer, sizeof(buffer), file.c_str());
         GenerateSpecFile = (buffer[0] == '1');
         GetPrivateProfileString(section, "CloseTheProgramWithoutPause", "", buffer, sizeof(buffer), file.c_str());
         CloseTheProgramWithoutPause = (buffer[0] == '1');
    }
    else 
    {
        WritePrivateProfileString(section, "RemoveBumpFromFilename", RemoveBumpFromFilename ? "1" : "0", file.c_str());
        WritePrivateProfileString(section, "GenerateSpecFile", GenerateSpecFile ? "1" : "0", file.c_str());
        WritePrivateProfileString(section, "CloseTheProgramWithoutPause", CloseTheProgramWithoutPause ? "1" : "0", file.c_str());
    }
    std::cout << "RemoveBumpFromFilename\t\t" << ((RemoveBumpFromFilename) ? "True" : "False") << std::endl;
    std::cout << "GenerateSpecFile\t\t" << ((GenerateSpecFile)?"True" :"False") << std::endl;
    std::cout << "CloseTheProgramWithoutPause\t" << ((CloseTheProgramWithoutPause) ? "True" : "False") << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << dye::red("There are no files to convert. Drag and drop files onto the program to start converting.") << std::endl;
        system("pause");
        return 0;
    }
    
    std::string argv_str(argv[0]);
    std::string settings_file = argv_str + "/../settings.ini";

    Settings_Initialise(settings_file);

    FreeImage_Initialise();

    for (int i = 0; i < argc-1; i++)
    {
        ProcessFile(argv[i+1], i+1, argc-1);
    }

    if (!CloseTheProgramWithoutPause) system("pause");
    FreeImage_DeInitialise();
    return 0;
}