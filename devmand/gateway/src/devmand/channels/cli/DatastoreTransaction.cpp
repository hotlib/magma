// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/DatastoreTransaction.h>
#include <magma_logging.h>
#include <boost/algorithm/string.hpp>

namespace devmand {
namespace channels {
namespace cli {

    shared_ptr<Entity> DatastoreTransaction::read(string path) {
        (void)path;
        return shared_ptr<Entity>();
    }

    void DatastoreTransaction::delete2(string path) {
        (void)path;
    }

    void DatastoreTransaction::write(string path, shared_ptr<Entity> entity) {
        (void)path;
        (void)entity;
    }

    void DatastoreTransaction::create(shared_ptr<Entity> entity) {
                (void)entity;

        LeafVector leafs;
        createLeafs(entity, string(""), leafs);
        for (const auto &leaf : leafs) {
            char * data = const_cast<char*>(leaf.second.c_str());
            lyd_new_path(root, NULL, leaf.first.c_str(), data, LYD_ANYDATA_STRING, LYD_PATH_OPT_UPDATE);
        }
        //lyd_node *root = lyd_new_path(NULL, ctx, "/openconfig-interfaces:interfaces", NULL, LYD_ANYDATA_STRING, LYD_PATH_OPT_UPDATE);

        //MLOG(MINFO) << "ENTITY segment path: " << entity->get_segment_path() << " |ENTITY full path: " << entity->get_absolute_path();
//        if(entity->get_name_leaf_data().size() > 0){
//            MLOG(MINFO) << "Entita " << entity->get_segment_path() << " ma data, hodnoty: ";
//            for (const auto &data : entity->get_name_leaf_data()) {
//                MLOG(MINFO) << "string: " << data.first << " leaf data: " << data.second.value;
//            }
//
//        }
//
//        for (const auto &child : entity->get_children()) {
//            //MLOG(MINFO) << "idem vypisat: " << child.first;
//            create(child.second);
//        }

       // MLOG(MINFO) << "relativna cesta entity ====================" << entity->get_segment_path();
//
//        lyd_node *pNode;
////        if(!entity->has_data()){
////            pNode = lyd_new_path(root, NULL, "/openconfig-interfaces:interfaces/interface/name", NULL, LYD_ANYDATA_STRING,
////                                           LYD_PATH_OPT_DFLT);
////        }
//            pNode = lyd_new(root, NULL, "whatever");
////
//        char value[] = "0/66";
//        //lyd_new_path(root, ctx, "bar/hi", value, LYD_ANYDATA_STRING, LYD_PATH_OPT_UPDATE );
//        lyd_new(root, NULL, "interface");
//        //lyd_new_path(root, ctx, "interface/name", value, LYD_ANYDATA_STRING, LYD_PATH_OPT_UPDATE);
//        lyd_new_path(root, ctx, "interface/name", value, LYD_ANYDATA_STRING, 0);
//        //lyd_new_leaf(ifacepNode, NULL, "name", value );
//        //lyd_new(pNode, NULL, "config");
        char * buff;
        lyd_print_mem(&buff, root, LYD_XML, 0 );
        MLOG(MINFO) << "buff ====================>>>>>> " << buff;
//      //  (void)pNode;
//        if(root == NULL)
//        MLOG(MINFO) << "==================== ROOT JE NULLL "; // << // << " obsah " << pNode->attr->name;
////        else
////            MLOG(MINFO) << " NOOOT ==================== added ";

    }

    void DatastoreTransaction::commit() {

    }

    DatastoreTransaction::DatastoreTransaction() {
        ctx = ly_ctx_new("/usr/share/openconfig@0.1.6/", 0);
        ly_ctx_load_module(ctx, "iana-if-type", NULL);
        const lys_module *module = ly_ctx_load_module(ctx, "openconfig-interfaces", NULL);
        //const lys_module *module = ly_ctx_load_module(ctx, "defaults", NULL);
        root = lyd_new(NULL, module, "interfaces");
        MLOG(MINFO) << (root == NULL ? "NULLLLLLLLLLLL11" : " NIEEEE NULLLL11");
    }

    void DatastoreTransaction::createLeafs(shared_ptr<Entity> entity, string init, LeafVector & leafs) {
        string prefix = init + "/" + entity->get_segment_path();
        std::vector<string> strs;
        if(entity->get_name_leaf_data().size() > 0){

            for (const auto &data : entity->get_name_leaf_data()) {

                boost::split(strs, data.first, boost::is_any_of(" "));
                string leafPath = prefix + "/" + strs[0];
                string leafData = data.second.value;
                leafs.emplace_back(std::make_pair(leafPath, leafData));
                //MLOG(MINFO) << "leafPath " <<  << " leafData: " << ;

            }
        }

        for (const auto &child : entity->get_children()) {
            //MLOG(MINFO) << "idem vypisat: " << child.first;
            createLeafs(child.second, prefix, leafs);
        }
    }
} // namespace cli
} // namespace channels
} // namespace devmand