#include <core/path.h>
#include "hair_asset.h"
#include "hair_importer.h"

int main()
{
    Path::register_protocol("content", std::string(PROJECT_DIR) + "/content/");
    Path::register_protocol("hair", std::string(PROJECT_DIR) + "/content/hair/");

    HairAsset* hair_asset = load_hair_asset(Path::fix_path("hair://hair_01.abc"));
    if (hair_asset)
        delete hair_asset;

    return 0;
}