---
blueprint_schema: tavernweave/domain-blueprint/v1
project_id: ai-npc-knowledge-grant
blueprint_id: BP-02
parent_id: BP-ROOT
status: completed
updated: 2026-10-20
---

# BP-02 · NPC 人格反应

## 输入与边界

输入为已解锁知识包、NPC 固定人格、可选当前情绪。采用延迟接收，不在任务完成时遍历全部 NPC；不调用模型，不修改玩家授权，不让情绪永久改变人格。

## 输出与证据

- `UNpcPersonalityProfile`：共情、好奇、谨慎、秩序忠诚、攻击性。
- `UKnowledgeReactionLibrary`：确定性反应评估。
- `UNpcKnowledgeReceiverComponent`：Authority 侧延迟接收、幂等与反应记忆快照。
- 输出兴趣、认可、威胁、强度、台词语气、行为意图和有界情绪建议量。

退出条件：锁定消息无反应；同消息不同人格产生不同结果；不同消息不同结果；当前情绪改变强度但不修改人格；重复消息幂等；快照恢复有效。

## 停止条件

需要模型调用、关系系统、阵营推理、人格学习或大规模 NPC 离线模拟。

## 下一道门

UE 5.7 编译与两项新增 Automation Test 已通过；停在驾驶员验收。
