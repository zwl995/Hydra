#pragma once
#include "kimera_dsg_builder/incremental_types.h"
#include "kimera_dsg_builder/object_finder.h"
#include "kimera_dsg_builder/semantic_ros_publishers.h"

#include <kimera_dsg/dynamic_scene_graph.h>
#include <kimera_dsg/node_symbol.h>
#include <kimera_semantics/semantic_integrator_base.h>

#include <dynamic_reconfigure/server.h>
#include <kimera_dsg_builder/DsgBuilderConfig.h>
#include <ros/ros.h>

#include <memory>
#include <mutex>

namespace kimera {
namespace incremental {

using kimera_dsg_builder::DsgBuilderConfig;

class MeshSegmenter {
 public:
  using LabelIndices = std::map<uint8_t, std::vector<size_t>>;
  using MeshVertexCloud = pcl::PointCloud<pcl::PointXYZRGBA>;
  using ObjectCloudPublishers = SemanticRosPublishers<uint8_t, MeshVertexCloud>;
  using ObjectCluster = Cluster<pcl::PointXYZRGBA>;

  explicit MeshSegmenter(const ros::NodeHandle& nh,
                         const MeshVertexCloud::Ptr& active_vertices);

  virtual ~MeshSegmenter() = default;

  bool detectObjects(const SharedDsgInfo::Ptr& dsg,
                     const std::vector<size_t>& active_indices,
                     const std::optional<Eigen::Vector3d>& pos);

  inline std::unordered_set<NodeId> getObjectsToCheckForPlaces() const {
    return objects_to_check_for_places_;
  }

  void pruneObjectsToCheckForPlaces(const DynamicSceneGraph& graph);

 private:
  void archiveOldObjects(const DynamicSceneGraph& graph, double latest_timestamp);

  LabelIndices getLabelIndices(const std::vector<size_t>& indices) const;

  void updateGraph(DynamicSceneGraph& graph,
                   const ObjectClusters& clusters,
                   uint8_t label,
                   double timestamp);

  void addObjectToGraph(DynamicSceneGraph& graph,
                        const ObjectCluster& cluster,
                        uint8_t label,
                        double timestamp);

  void updateObjectInGraph(DynamicSceneGraph& graph,
                           const ObjectCluster& cluster,
                           const SceneGraphNode& node,
                           double timestamp);

  void objectFinderConfigCb(DsgBuilderConfig& config, uint32_t);

  void publishActiveVertices(const std::vector<size_t>& indices) const;

  void publishObjectClouds(const LabelIndices& label_indices) const;

 private:
  ros::NodeHandle nh_;

  MeshVertexCloud::Ptr full_mesh_vertices_;

  NodeSymbol next_node_id_;
  std::unique_ptr<ObjectFinder> object_finder_;

  double active_object_horizon_s_;
  double active_index_horizon_m_;
  std::map<uint8_t, std::set<NodeId>> active_objects_;
  std::map<NodeId, double> active_object_timestamps_;
  std::unordered_set<NodeId> objects_to_check_for_places_;

  std::set<uint8_t> object_labels_;
  bool enable_active_mesh_pub_;
  bool enable_segmented_mesh_pub_;

  BoundingBox::Type bounding_box_type_;

  ros::Publisher active_mesh_vertex_pub_;
  std::unique_ptr<ObjectCloudPublishers> segmented_mesh_vertices_pub_;

  // TODO(nathan) think about replacing this
  SemanticIntegratorBase::SemanticConfig semantic_config_;

  dynamic_reconfigure::Server<DsgBuilderConfig> rqt_server_;
  dynamic_reconfigure::Server<DsgBuilderConfig>::CallbackType rqt_callback_;
};

}  // namespace incremental
}  // namespace kimera
