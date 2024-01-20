#include "hair_importer.h"
#include "Alembic/AbcGeom/All.h"
#include "Alembic/AbcCoreFactory/IFactory.h"
#include "Alembic/Abc/IArchive.h"
#include "Alembic/Abc/IObject.h"

HairAsset* load_hair_asset(const std::string& file_path)
{
    Alembic::AbcCoreFactory::IFactory factory;
    Alembic::AbcCoreFactory::IFactory::CoreType compression_type = Alembic::AbcCoreFactory::IFactory::kUnknown;
    Alembic::Abc::IArchive archive;
    Alembic::Abc::IObject top_object;
    return nullptr;
}