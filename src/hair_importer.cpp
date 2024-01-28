#include "hair_importer.h"
#include "hair_asset.h"
#include "Alembic/AbcGeom/All.h"
#include "Alembic/AbcCoreFactory/IFactory.h"
#include "Alembic/Abc/IArchive.h"
#include "Alembic/Abc/IObject.h"
#include <glm/glm.hpp>

void parse_object(const Alembic::Abc::IObject& abc_object, float frame_time, const glm::mat4& parent_matrix, HairAsset::Builder* builder)
{
    const Alembic::Abc::MetaData object_meta_data = abc_object.getMetaData();
    const uint32_t num_children = abc_object.getNumChildren();

    glm::mat4 local_matrix = parent_matrix;
    if (Alembic::AbcGeom::ICurves::matches(object_meta_data))
    {
        Alembic::AbcGeom::ICurves curves = Alembic::AbcGeom::ICurves(abc_object, Alembic::Abc::kWrapExisting);
        Alembic::Abc::ISampleSelector sample_selector((Alembic::Abc::chrono_t) frame_time);
        Alembic::AbcGeom::ICurves::schema_type::Sample sample = curves.getSchema().getValue(sample_selector);

        Alembic::Abc::P3fArraySamplePtr positions = sample.getPositions();
        Alembic::Abc::Int32ArraySamplePtr num_vertices = sample.getCurvesNumVertices();
        uint32_t num_curves = sample.getNumCurves();
        int global_index =0;
        for (int curve_index = 0; curve_index < num_curves; ++curve_index)
        {
            const int curve_num_vertices = (*num_vertices)[curve_index];
            int strand_id = builder->add_strand();
            builder->set_stand_vertex_count(strand_id, curve_num_vertices);
            builder->set_stand_vertex_offset(strand_id, builder->get_num_vertices());

            for (int point_index = 0; point_index < curve_num_vertices; ++point_index, ++global_index)
            {
                int vertex_id = builder->add_vertex();
                Alembic::Abc::P3fArraySample::value_type position = (*positions)[global_index];
                builder->set_vertex_position(vertex_id, glm::vec3(position.x, position.y, position.z));
            }
        }
    }

    if (num_children > 0)
    {
        for (int i = 0; i < num_children; ++i)
        {
            parse_object(abc_object.getChild(i), frame_time, local_matrix, builder);
        }
    }
}

HairAsset* load_hair_asset(const std::string& file_path)
{
    Alembic::AbcCoreFactory::IFactory factory;
    Alembic::AbcCoreFactory::IFactory::CoreType compression_type = Alembic::AbcCoreFactory::IFactory::kUnknown;
    Alembic::Abc::IArchive archive;
    Alembic::Abc::IObject top_object;

    factory.setPolicy(Alembic::Abc::ErrorHandler::kThrowPolicy);
    factory.setOgawaNumStreams(12);

    archive = factory.getArchive(file_path.c_str(), compression_type);
    if (!archive.valid())
    {
        return nullptr;
    }

    top_object = Alembic::Abc::IObject(archive, Alembic::Abc::kTop);
    if (!top_object.valid())
    {
        return nullptr;
    }

    HairAsset::Builder builder;
    glm::mat4 parent_matrix = glm::mat4(1.0f);
    parse_object(top_object, 0.0f, parent_matrix, &builder);
    return builder.build();
}