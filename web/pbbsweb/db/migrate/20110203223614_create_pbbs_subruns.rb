class CreatePbbsSubruns < ActiveRecord::Migration
  def self.up
    create_table :pbbs_subruns do |t|

      t.timestamps
    end
  end

  def self.down
    drop_table :pbbs_subruns
  end
end
